#pragma once

// ================================================================
//  ComponentArray.h
//  High-performance packed storage for a single component type.
//
//  Design:
//  - Uses ge::containers::DynamicArray for contiguous memory.
//  - Maintains a packed layout (no gaps) for cache-friendly iteration.
//  - Maps Entity Index <-> Component Index for O(1) lookups.
//  - Swap-with-last removal keeps entries packed.
// ================================================================

#include "Entity.h"
#include "../containers/dynamic_array.h"
#include "../debug/assert.h"
#include <unordered_map>

namespace ge {
namespace ecs
{

/**
 * @brief Base interface for polymorphic component arrays.
 * Allows the World to clean up all storage without knowing their types.
 */
class IComponentArray
{
public:
    virtual ~IComponentArray() = default;
    virtual void EntityDestroyed(Entity e) = 0;
    virtual void RemoveData(Entity e) = 0;
};

/**
 * @brief Packed storage for component of type T.
 */
template <typename T>
class ComponentArray final : public IComponentArray
{
public:
    explicit ComponentArray(memory::IAllocator* allocator = nullptr)
        : components_(allocator)
    {
    }

    /// Add a component to an entity.
    void InsertData(Entity e, T data)
    {
        const std::uint32_t entityIdx = e.GetIndex();
        GE_ASSERT(entityToComponentMap_.find(entityIdx) == entityToComponentMap_.end(), 
                  "Entity already has this component type!");

        // Put at the end of the packed array
        const std::size_t newIndex = components_.Size();
        entityToComponentMap_[entityIdx] = newIndex;
        componentToEntityMap_[newIndex]  = entityIdx;
        
        components_.Push(std::move(data));
        entities_.Push(e);
    }

    /// Remove component from an entity and keep array packed.
    void RemoveData(Entity e) override
    {
        const std::uint32_t entityIdx = e.GetIndex();
        auto it = entityToComponentMap_.find(entityIdx);
        if (it == entityToComponentMap_.end()) return;

        const std::size_t indexToRemove = it->second;
        const std::size_t lastIndex     = components_.Size() - 1;

        // Move the last element into the hole
        if (indexToRemove != lastIndex)
        {
            components_[indexToRemove] = std::move(components_[lastIndex]);
            entities_[indexToRemove]   = std::move(entities_[lastIndex]);

            // Update maps for the moved element
            const std::uint32_t movedEntityIdx = entities_[indexToRemove].GetIndex();
            entityToComponentMap_[movedEntityIdx] = indexToRemove;
            componentToEntityMap_[indexToRemove]  = movedEntityIdx;
        }

        // Pop last element
        components_.Pop();
        entities_.Pop();
        entityToComponentMap_.erase(entityIdx);
        componentToEntityMap_.erase(lastIndex);
    }

    /// Get reference to an entity's component.
    [[nodiscard]] T& GetData(Entity e)
    {
        const std::uint32_t entityIdx = e.GetIndex();
        auto it = entityToComponentMap_.find(entityIdx);
        GE_ASSERT(it != entityToComponentMap_.end(), 
                  "Retrieving component that doesn't exist on entity.");
        return components_[it->second];
    }

    /// Check if entity has this component.
    [[nodiscard]] bool HasData(Entity e) const
    {
        return entityToComponentMap_.find(e.GetIndex()) != entityToComponentMap_.end();
    }

    /// Notification from EntityManager that an entity was destroyed.
    void EntityDestroyed(Entity e) override
    {
        RemoveData(e);
    }

    // ── Internal access for Systems ─────────────────────────────

    [[nodiscard]] std::size_t Size() const noexcept { return components_.Size(); }
    [[nodiscard]] const containers::DynamicArray<Entity>& GetEntities() const noexcept { return entities_; }

private:
    containers::DynamicArray<T>      components_;
    containers::DynamicArray<Entity> entities_;

    // Mapping: Entity Index -> Index in components_ array
    std::unordered_map<std::uint32_t, std::size_t> entityToComponentMap_;
    
    // Mapping: Index in components_ array -> Entity Index
    std::unordered_map<std::size_t, std::uint32_t> componentToEntityMap_;
};

} // namespace ecs
} // namespace ge
