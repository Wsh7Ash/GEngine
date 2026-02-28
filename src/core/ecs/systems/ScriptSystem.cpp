#include "ScriptSystem.h"
#include "../../debug/log.h"
#include "../World.h"
#include "../components/NativeScriptComponent.h"


namespace ge {
namespace ecs {

void ScriptSystem::Update(World &world, float ts) {
  for (auto const &entity : entities) {
    if (!world.HasComponent<NativeScriptComponent>(entity)) {
      GE_LOG_CRITICAL("FATAL: Entity [idx=%u, ver=%u] is in ScriptSystem but "
                      "has no NativeScriptComponent! Skipping!",
                      entity.GetIndex(), entity.GetVersion());
      continue;
    }

    auto &nsc = world.GetComponent<NativeScriptComponent>(entity);

    // 1. Instantiate if needed
    if (!nsc.instance) {
      // Skip if InstantiateScript is not bound (e.g. after deserialization)
      if (!nsc.InstantiateScript)
        continue;

      nsc.instance = nsc.InstantiateScript();
      nsc.instance->entity_ = entity;
      nsc.instance->world_ = &world;
      nsc.instance->OnCreate();
    }

    // 2. Update
    nsc.instance->OnUpdate(ts);
  }
}

} // namespace ecs
} // namespace ge
