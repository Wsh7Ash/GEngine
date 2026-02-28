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

namespace ge {
namespace ecs {

class World {
public:
  explicit World(memory::IAllocator *allocator = nullptr)
      : allocator_(allocator ? allocator : memory::GetDefaultAllocator()),
        entityManager_(10000), // Default capacity
        systemManager_(std::make_unique<SystemManager>()) {
    entitySignatures_.resize(10000);
  }

  // ── Entity management ───────────────────────────────────────

  [[nodiscard]] Entity CreateEntity() {
    Entity e = entityManager_.CreateEntity();
    allEntities_.push_back(e);
    return e;
  }

  void DestroyEntity(Entity e) {
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
    systemManager_->EntityDestroyed(e);
  }

  void Clear() {
    // Collect active entities directly from our tracked list
    auto alive = allEntities_;
    for (auto &e : alive) {
      DestroyEntity(e);
    }
  }

  [[nodiscard]] bool IsAlive(Entity e) const noexcept {
    return entityManager_.IsAlive(e);
  }

  // ── Component management ─────────────────────────────────────

  template <typename T> void AddComponent(Entity e, T component) {
    GetComponentArray<T>()->InsertData(e, std::move(component));

    // Update signature
    auto signature = entitySignatures_[e.GetIndex()];
    signature.set(GetComponentTypeID<T>(), true);
    entitySignatures_[e.GetIndex()] = signature;

    systemManager_->EntitySignatureChanged(e, signature);
  }

  template <typename T> void RemoveComponent(Entity e) {
    GetComponentArray<T>()->RemoveData(e);

    // Update signature
    auto signature = entitySignatures_[e.GetIndex()];
    signature.set(GetComponentTypeID<T>(), false);
    entitySignatures_[e.GetIndex()] = signature;

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

  // ── Queries ──────────────────────────────────────────────────

  template <typename... Components>
  [[nodiscard]] EntityQuery<Components...> Query() {
    return ecs::EntityQuery<Components...>(this);
  }

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
