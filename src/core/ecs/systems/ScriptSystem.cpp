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
    // 1. Instantiate if needed
    if (!nsc.instance) {
      GE_LOG_INFO("ScriptSystem: Entity %d needs instantiation. ScriptName=%s, InstantiateScript bound=%d", 
                  entity.GetIndex(), nsc.ScriptName.c_str(), (bool)nsc.InstantiateScript);
      fflush(stdout);
      
      // Skip if InstantiateScript is not bound (e.g. after deserialization)
      if (!nsc.InstantiateScript) {
        GE_LOG_INFO("ScriptSystem: Skipping Entity %d (no InstantiateScript)", entity.GetIndex());
        fflush(stdout);
        continue;
      }

      GE_LOG_INFO("ScriptSystem: Calling InstantiateScript for Entity %d", entity.GetIndex());
      fflush(stdout);
      nsc.instance = nsc.InstantiateScript();
      GE_LOG_INFO("ScriptSystem: Instantiated ScriptableEntity for Entity %d", entity.GetIndex());
      fflush(stdout);
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
