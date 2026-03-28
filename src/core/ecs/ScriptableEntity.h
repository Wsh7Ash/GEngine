#pragma once

#include "Entity.h"
#include "World.h"
#include "../math/VecTypes.h"
#include "components/InputStateComponent.h"

namespace ge {
namespace ecs {

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

  // Input State Hooks
  virtual void OnInputModeChanged([[maybe_unused]] InputMode mode) {}
  virtual void OnMovementStateChanged([[maybe_unused]] MovementState newState, [[maybe_unused]] MovementState oldState) {}
  virtual void OnMovementInput([[maybe_unused]] const InputAxis& moveAxis) {}
  virtual void OnLookInput([[maybe_unused]] const InputAxis& lookAxis) {}
  virtual void OnJumpPressed() {}
  virtual void OnJumpReleased() {}
  virtual void OnCrouchPressed() {}
  virtual void OnCrouchReleased() {}
  virtual void OnSprintPressed() {}
  virtual void OnSprintReleased() {}
  virtual void OnCustomAction([[maybe_unused]] int actionIndex, [[maybe_unused]] bool isPressed) {}

  // Transform Interpolation Hook
  virtual void OnTransformInterpolate([[maybe_unused]] const Math::Vec3f& interpolatedPos, [[maybe_unused]] float alpha) {}

protected:
  virtual void OnCreate() {}
  virtual void OnUpdate([[maybe_unused]] float ts) {}
  virtual void OnDestroy() {}

private:
  Entity entity_;
  World *world_ = nullptr;

  friend class ScriptSystem;
  friend class ScriptRegistry;
};

} // namespace ecs
} // namespace ge
