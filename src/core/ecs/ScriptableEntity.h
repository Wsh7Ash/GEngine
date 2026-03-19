#pragma once

#include "Entity.h"
#include "World.h"
#include "../math/VecTypes.h"

namespace ge {
namespace ecs {

/**
 * @brief Base class for all native scripts.
 *
 * Users should inherit from this class to define custom entity behavior.
 */
class ScriptableEntity {
public:
  virtual ~ScriptableEntity() = default;

  template <typename T> T &GetComponent() {
    return world_->GetComponent<T>(entity_);
  }

  template <typename T> bool HasComponent() {
    return world_->HasComponent<T>(entity_);
  }

  // Input helpers
  bool IsKeyPressed(int keyCode);
  bool IsMouseButtonPressed(int button);
  ::Math::Vec2f GetMousePosition();

  // Lifecycle
  void Destroy();

  // Serialization hooks
  virtual void OnSerialize([[maybe_unused]] void *jsonRoot) {}
  virtual void OnDeserialize([[maybe_unused]] void *jsonRoot) {}

  // Utility helpers
  void LogInfo(const char *msg);
  void LogWarning(const char *msg);
  void LogError(const char *msg);

  // Physics Callbacks
  virtual void OnCollisionEnter([[maybe_unused]] Entity other) {}
  virtual void OnCollisionExit([[maybe_unused]] Entity other) {}
  virtual void OnTriggerEnter([[maybe_unused]] Entity other) {}
  virtual void OnTriggerExit([[maybe_unused]] Entity other) {}

protected:
  virtual void OnCreate() {}
  virtual void OnUpdate([[maybe_unused]] float ts) {}
  virtual void OnDestroy() {}

private:
  Entity entity_;
  World *world_ = nullptr;

  friend class ScriptSystem;
};

} // namespace ecs
} // namespace ge
