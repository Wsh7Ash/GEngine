#include "ScriptSystem.h"
#include "../../debug/log.h"
#include "../World.h"
#include "../components/NativeScriptComponent.h"
#include "../ScriptRegistry.h"
#include "../../scripting/ScriptEngine.h"
#include "../../scripting/ManagedWorldBridge.h"


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

    if (!nsc.instance) {
      if (!nsc.InstantiateScript) {
        continue;
      }

      nsc.instance = nsc.InstantiateScript();
      nsc.instance->entity_ = entity;
      nsc.instance->world_ = &world;
      nsc.instance->OnCreate();
    }

    nsc.instance->OnUpdate(ts);
  }

  UpdateManagedScripts(world, ts);
}

void ScriptSystem::UpdateManagedScripts(World& world, float ts) {
    auto managedEntities = world.Query<scripting::ManagedScriptComponent, TransformComponent>();
    
    for (auto entity : managedEntities) {
        auto& msc = world.GetComponent<scripting::ManagedScriptComponent>(entity);
        
        if (!msc.instance) {
            InstantiateManagedScript(world, entity, msc);
        }
        
        if (msc.instance) {
            msc.instance->OnUpdate(ts);
        }
    }
}

void ScriptSystem::InstantiateManagedScript(World& world, Entity entity, scripting::ManagedScriptComponent& msc) {
    msc.entity = entity;
    msc.world = &world;
    
    if (msc.instance) {
        msc.instance->OnCreate();
        GE_LOG_INFO("Managed script instantiated: %s", msc.className.c_str());
    }
}

void ScriptSystem::ReloadScripts(World& world) {
    ScriptRegistry::ReloadAll(world);
}

} // namespace ecs
} // namespace ge