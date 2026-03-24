#pragma once
// ================================================================
//  World.h
//  Central hub for the ECS.
//
//  Coordinates entities, components, and systems.
// ================================================================

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
#include "components/IDComponent.h"
#include "components/RelationshipComponent.h"

namespace ge {
namespace ecs {

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

  [[nodiscard]] Entity CreateEntity() {
    Entity e = entityManager_.CreateEntity();
    allEntities_.push_back(e);
    isDirty_ = true;
    
    UUID uuid;
    AddComponent<IDComponent>(e, IDComponent{uuid});
    entityByUUID_[uuid] = e;
    
    return e;
  }

  [[nodiscard]] Entity CreateEntityWithUUID(UUID uuid) {
    Entity e = entityManager_.CreateEntity();
    allEntities_.push_back(e);
    isDirty_ = true;
    
    AddComponent<IDComponent>(e, IDComponent{uuid});
    entityByUUID_[uuid] = e;
    
    return e;
  }

  void DestroyEntity(Entity e) {
    if (!IsAlive(e)) return;

    if (HasComponent<IDComponent>(e)) {
      entityByUUID_.erase(GetComponent<IDComponent>(e).ID);
    }

    // Recursively destroy children
    if (HasComponent<RelationshipComponent>(e)) {
      auto &rc = GetComponent<RelationshipComponent>(e);
      // Make a copy since the Children vector will be modified as children are destroyed
      auto children = rc.Children;
      for (auto child : children) {
        DestroyEntity(child);
      }
    }

    // Remove from parent if exists
    if (HasComponent<RelationshipComponent>(e)) {
      Entity parent = GetComponent<RelationshipComponent>(e).Parent;
      if (parent != INVALID_ENTITY && HasComponent<RelationshipComponent>(parent)) {
        auto &parentRc = GetComponent<RelationshipComponent>(parent);
        auto &children = parentRc.Children;
        children.erase(std::remove(children.begin(), children.end(), e), children.end());
      }
    }

    auto it = std::find(allEntities_.begin(), allEntities_.end(), e);
    if (it != allEntities_.end()) {
      *it = allEntities_.back();
      allEntities_.pop_back();
    }

    entityManager_.DestroyEntity(e);

    // Notify all component storages
    for (auto &storage : componentArrays_) {
      if (storage) {
        storage->EntityDestroyed(e);
      }
    }

    // Reset signature and notify systems
    entitySignatures_[e.GetIndex()].reset();
    isDirty_ = true;
    systemManager_->EntityDestroyed(e);
  }

  void SetParent(Entity child, Entity parent) {
    if (child == parent || child == INVALID_ENTITY) return;

    // Ensure RelationshipComponent exists on child
    if (!HasComponent<RelationshipComponent>(child)) {
      AddComponent<RelationshipComponent>(child, RelationshipComponent{});
    }

    auto &childRc = GetComponent<RelationshipComponent>(child);
    
    // Remove from old parent
    if (childRc.Parent != INVALID_ENTITY && HasComponent<RelationshipComponent>(childRc.Parent)) {
      auto &oldParentRc = GetComponent<RelationshipComponent>(childRc.Parent);
      auto &children = oldParentRc.Children;
      children.erase(std::remove(children.begin(), children.end(), child), children.end());
    }

    childRc.Parent = parent;

    // Add to new parent
    if (parent != INVALID_ENTITY) {
      if (!HasComponent<RelationshipComponent>(parent)) {
        AddComponent<RelationshipComponent>(parent, RelationshipComponent{});
      }
      auto &parentRc = GetComponent<RelationshipComponent>(parent);
      parentRc.Children.push_back(child);
    }
    
    isDirty_ = true;
  }

  void Clear() {
    // Destroy only root entities (RelationshipComponent::Parent == INVALID)
    // or entities without RelationshipComponent. DestroyEntity will recursively clean up children.
    auto alive = allEntities_;
    for (auto &e : alive) {
      if (IsAlive(e)) {
        bool isRoot = true;
        if (HasComponent<RelationshipComponent>(e)) {
          isRoot = (GetComponent<RelationshipComponent>(e).Parent == INVALID_ENTITY);
        }
        
        if (isRoot) {
          DestroyEntity(e);
        }
      }
    }
  }

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
