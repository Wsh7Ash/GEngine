#pragma once

// ================================================================
//  EntityManager.h
//  Handles creation, destruction, and validation of entities.
//
//  Wraps ge::containers::HandlePool to provide O(1) entity
//  allocation and recycle indices using 32-bit versions.
// ================================================================

#include "Entity.h"
#include <vector>

namespace ge {
namespace ecs
{

class EntityManager
{
public:
    explicit EntityManager(std::uint32_t capacity = 10000)
        : pool_(capacity)
    {
    }

    /// Create a new unique entity.
    [[nodiscard]] Entity CreateEntity()
    {
        return pool_.Allocate();
    }

    /// Mark an entity as dead and recycle its index.
    void DestroyEntity(Entity e)
    {
        pool_.Release(e);
    }

    /// Check if an entity handle is still valid (not stale or destroyed).
    [[nodiscard]] bool IsAlive(Entity e) const noexcept
    {
        return pool_.IsValid(e);
    }

    [[nodiscard]] std::uint32_t GetCapacity()  const noexcept { return pool_.GetCapacity(); }
    [[nodiscard]] std::uint32_t GetEntityCount() const noexcept { return pool_.GetUsedCount(); }

private:
    containers::HandlePool<EntityMarker> pool_;
};

} // namespace ecs
} // namespace ge
