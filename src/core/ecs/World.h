#pragma once

// ================================================================
//  World.h
//  Central hub for the ECS.
//
//  Coordinates entities, components, and systems.
// ================================================================

/**
 * @defgroup ecs ECS (Entity Component System)
 * @brief Core ECS framework for GEngine.
 * 
 * The ECS system provides a data-oriented architecture for managing game entities.
 * 
 * @{
 *   @defgroup ecs_core Core
 *   @brief Core ECS concepts and types.
 * @}
 * @{
 *   @defgroup ecs_components Components
 *   @brief Built-in ECS components.
 * @}
 * @{
 *   @defgroup ecs_systems Systems
 *   @brief Built-in ECS systems.
 * @}
 */

#include "../memory/allocator.h"
#include "ComponentArray.h"
#include "ComponentRegistry.h"
#include "EntityManager.h"
#include "Query.h"
#include "SystemManager.h"
#include <array>
#include <memory>
#include <tuple>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include "components/IDComponent.h"
#include "EntityCloner.h"

namespace ge {
namespace ecs {
    struct TransformComponent;
    struct MeshComponent;
    struct ModelComponent;
    struct LightComponent;
    struct PostProcessComponent;
    struct TagComponent;
    struct SpriteComponent;
    struct AnimatorComponent;
    struct SkyboxComponent;
}
}
#include "components/RelationshipComponent.h"

namespace ge {
namespace ecs {

/**
 * @brief Central hub for the ECS architecture.
 * 
 * The World class is the main entry point for all ECS operations. It manages:
 * - Entity creation and destruction
 * - Component storage and retrieval
 * - System registration and updates
 * - Entity hierarchies (parent/child relationships)
 * 
 * @note Thread Safety: World is NOT thread-safe. All operations must occur
 *       on the main thread. See docs/THREADING_CONTRACT.md for details.
 * 
 * @par Example:
 * @code
 * ecs::World world;
 * 
 * // Create entities with components
 * auto entity = world.CreateEntity();
 * world.AddComponent(entity, TransformComponent{{1.0f, 0.0f, 0.0f}, {}, {1,1,1}});
 * 
 * // Query entities
 * for (auto [transform, mesh] : world.Query<TransformComponent, MeshComponent>()) {
 *     // Process entities with both components
 * }
 * @endcode
 * 
 * @see docs/THREADING_CONTRACT.md
 * 
 * @ingroup ecs_core
 */
class World {
  template <typename... Comps> friend class EntityQuery;

public:
  explicit World(memory::IAllocator *allocator = nullptr)
      : allocator_(allocator ? allocator : memory::GetDefaultAllocator()),
        entityManager_(10000), // Default capacity
        systemManager_(std::make_unique<SystemManager>()) {
    entitySignatures_.resize(10000);
    isDirty_ = true; // Initial structural change
  }

  // ── Entity management ───────────────────────────────────────

  [[nodiscard]] Entity CreateEntity();

  [[nodiscard]] Entity CreateEntityWithUUID(UUID uuid);

  void DestroyEntity(Entity e);

  void SetParent(Entity child, Entity parent);

  void Clear();

  [[nodiscard]] bool IsAlive(Entity e) const noexcept {
    return entityManager_.IsAlive(e);
  }

  [[nodiscard]] Entity GetEntityByUUID(UUID uuid) const {
    auto it = entityByUUID_.find(uuid);
    if (it != entityByUUID_.end()) {
      return it->second;
    }
    return INVALID_ENTITY;
  }

  [[nodiscard]] Entity CloneEntity(Entity entity, CloneOptions options = CloneOptions::GenerateNewIDs);

  // ── Component management ─────────────────────────────────────

  template <typename T> void AddComponent(Entity e, T component) {
    GetComponentArray<T>()->InsertData(e, std::move(component));

    // Update signature
    auto signature = entitySignatures_[e.GetIndex()];
    signature.set(GetComponentTypeID<T>(), true);
    entitySignatures_[e.GetIndex()] = signature;

    isDirty_ = true;
    systemManager_->EntitySignatureChanged(e, signature);
  }

  template <typename T> void RemoveComponent(Entity e) {
    GetComponentArray<T>()->RemoveData(e);

    // Update signature
    auto signature = entitySignatures_[e.GetIndex()];
    signature.set(GetComponentTypeID<T>(), false);
    entitySignatures_[e.GetIndex()] = signature;

    isDirty_ = true;
    systemManager_->EntitySignatureChanged(e, signature);
  }

  template <typename T> [[nodiscard]] T &GetComponent(Entity e) {
    return GetComponentArray<T>()->GetData(e);
  }

  // Helper accessors to avoid template specialization issues in some translation units
  TransformComponent& GetTransform(Entity e);
  MeshComponent& GetMesh(Entity e);
  ModelComponent& GetModel(Entity e);
  LightComponent& GetLight(Entity e);
  PostProcessComponent& GetPostProcess(Entity e);
  TagComponent& GetTag(Entity e);
  SpriteComponent& GetSprite(Entity e);
  AnimatorComponent& GetAnimator(Entity e);
  SkyboxComponent& GetSkybox(Entity e);
  RelationshipComponent& GetRelationship(Entity e);
  IDComponent& GetID(Entity e);

  bool HasAnimator(Entity e) const;
  bool HasMesh(Entity e) const;
  bool HasModel(Entity e) const;
  bool HasLight(Entity e) const;
  bool HasSprite(Entity e) const;
  bool HasSkybox(Entity e) const;
  bool HasRelationship(Entity e) const;

  // Component lookup by name (for managed script interop)
  bool HasComponentByName(Entity e, const char* componentName) const;
  void* GetComponentPointerByName(Entity e, const char* componentName);
  void AddComponentByName(Entity e, const char* componentName);
  void RemoveComponentByName(Entity e, const char* componentName);
  const char* GetComponentTypeName(ComponentTypeID id) const;

  static const char* GetComponentTypeNameStatic(ComponentTypeID id);

  template <typename T> [[nodiscard]] bool HasComponent(Entity e) const {
    const auto id = GetComponentTypeID<T>();
    if (id >= componentArrays_.size() || !componentArrays_[id])
      return false;

    return static_cast<ComponentArray<T> *>(componentArrays_[id].get())
        ->HasData(e);
  }

  // ── System management ───────────────────────────────────────

  template <typename T> std::shared_ptr<T> RegisterSystem() {
    return systemManager_->RegisterSystem<T>();
  }

  template <typename T> void SetSystemSignature(Signature signature) {
    systemManager_->SetSignature<T>(signature);
  }

  template <typename T> std::shared_ptr<T> GetSystem() {
    return systemManager_->GetSystem<T>();
  }

  // ── Queries ──────────────────────────────────────────────────

  template <typename... Components>
  [[nodiscard]] EntityQuery<Components...> Query() {
    return ecs::EntityQuery<Components...>(this);
  }

  [[nodiscard]] bool IsDirty() const noexcept { return isDirty_; }
  void ClearDirty() noexcept { isDirty_ = false; }
  void MarkDirty() noexcept { isDirty_ = true; }

  // ── Serialization ───────────────────────────────────────────────

  void Serialize(const std::string& filepath);
  void Deserialize(const std::string& filepath);

  std::string SerializeToString();
  void DeserializeFromString(const std::string& data);

private:
  /**
   * @brief Lazy-initializes and returns the storage for type T.
   */
  template <typename T> ComponentArray<T> *GetComponentArray() {
    const ComponentTypeID id = GetComponentTypeID<T>();
    GE_ASSERT(id < MAX_COMPONENTS, "Exceeded maximum component types!");

    if (!componentArrays_[id]) {
      componentArrays_[id] = std::make_unique<ComponentArray<T>>(allocator_);
    }

    return static_cast<ComponentArray<T> *>(componentArrays_[id].get());
  }

  memory::IAllocator *allocator_;
  EntityManager entityManager_;
  std::unique_ptr<SystemManager> systemManager_;

  // One storage array per unique component type ID.
  std::array<std::unique_ptr<IComponentArray>, MAX_COMPONENTS>
      componentArrays_{};

  // Tracks which components each entity has (using its index).
  std::vector<Signature> entitySignatures_;

  // Tracks all allocated entities to allow safe, exact cleanup
  std::vector<Entity> allEntities_;

  std::unordered_map<UUID, Entity> entityByUUID_;

  bool isDirty_ = false;
};

} // namespace ecs
} // namespace ge

// ================================================================
//  Query Implementation
// ================================================================

namespace ge {
namespace ecs {

template <typename... Components>
void EntityQuery<Components...>::Iterator::FindNext() {
  // The query iterates over the components of the FIRST type in the list.
  // Optimization: In a more advanced ECS, we'd pick the SMALLEST array.
  using FirstType =
      typename std::tuple_element<0, std::tuple<Components...>>::type;
  auto *storage = world_->template GetComponentArray<FirstType>();
  const auto &entities = storage->GetEntities();

  while (index_ < entities.Size()) {
    Entity e = entities[index_];
    // Check if entity has all OTHER components in the list
    if ((world_->template HasComponent<Components>(e) && ...)) {
      return;
    }
    ++index_;
  }
}

template <typename... Components>
Entity EntityQuery<Components...>::Iterator::operator*() const {
  using FirstType =
      typename std::tuple_element<0, std::tuple<Components...>>::type;
  auto *storage = world_->template GetComponentArray<FirstType>();
  return storage->GetEntities()[index_];
}

template <typename... Components>
typename EntityQuery<Components...>::Iterator
EntityQuery<Components...>::begin() {
  return Iterator(world_, 0);
}

template <typename... Components>
typename EntityQuery<Components...>::Iterator
EntityQuery<Components...>::end() {
  using FirstType =
      typename std::tuple_element<0, std::tuple<Components...>>::type;
  auto *storage = world_->template GetComponentArray<FirstType>();
  return Iterator(world_, storage->Size());
}

} // namespace ecs
} // namespace ge
