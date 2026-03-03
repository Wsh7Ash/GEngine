#include "ScriptableEntity.h"
#include "../debug/log.h"
#include "components/TagComponent.h"

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
} // namespace ecs
} // namespace ge