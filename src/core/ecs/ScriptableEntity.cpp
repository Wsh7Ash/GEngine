#include "ScriptableEntity.h"
#include "../debug/log.h"
#include "components/TagComponent.h"
#include "../platform/Input.h"

namespace ge {
namespace ecs {
void ScriptableEntity::LogInfo(const char *msg) {
  std::string tag = world_->HasComponent<TagComponent>(entity_)
                        ? world_->GetComponent<TagComponent>(entity_).tag
                        : "Unknown Entity";
  GE_LOG_INFO("[Script:{0}] {1}", tag.c_str(), msg);
}
void ScriptableEntity::LogWarning(const char *msg) {
  std::string tag = world_->HasComponent<TagComponent>(entity_)
                        ? world_->GetComponent<TagComponent>(entity_).tag
                        : "Unknown Entity";
  GE_LOG_WARNING("[Script:{0}] {1}", tag.c_str(), msg);
}
void ScriptableEntity::LogError(const char *msg) {
  std::string tag = world_->HasComponent<TagComponent>(entity_)
                        ? world_->GetComponent<TagComponent>(entity_).tag
                        : "Unknown Entity";
  GE_LOG_ERROR("[Script:{0}] {1}", tag.c_str(), msg);
}

bool ScriptableEntity::IsKeyPressed(int keyCode) {
  return platform::Input::IsKeyPressed(keyCode);
}

bool ScriptableEntity::IsMouseButtonPressed(int button) {
  return platform::Input::IsMouseButtonPressed(button);
}

::Math::Vec2f ScriptableEntity::GetMousePosition() {
  auto pos = platform::Input::GetMousePosition();
  return { pos.first, pos.second };
}

void ScriptableEntity::Destroy() {
  world_->DestroyEntity(entity_);
}
} // namespace ecs
} // namespace ge