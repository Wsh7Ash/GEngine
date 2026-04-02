#include "ScriptSystem.h"
#include "../../debug/log.h"
#include "../World.h"
#include "../components/NativeScriptComponent.h"
#include "../ScriptRegistry.h"
#include "../../scripting/ScriptEngine.h"
#include "../../scripting/ManagedWorldBridge.h"
#include "../../scripting/ManagedScriptComponent.h"
#include "../../scripting/NativeInterop.h"


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
            
            scripting::interop::ScriptManager_OnUpdate(ts);
        }
    }
}

void ScriptSystem::InstantiateManagedScript(World& world, Entity entity, scripting::ManagedScriptComponent& msc) {
    msc.entity = entity;
    msc.world = &world;
    
    if (msc.className.empty()) {
        GE_LOG_WARNING("ManagedScriptComponent has no class name for entity %llu", entity.value);
        return;
    }
    
    auto* wrapper = new scripting::ManagedScriptInstanceWrapper(nullptr, entity, &world);
    
    wrapper->SetCreateCallback([entityId = entity.value, className = msc.className]() {
        GE_LOG_INFO("Managed script created: %s for entity %llu", className.c_str(), entityId);
        scripting::interop::ScriptManager_OnCreate(entityId, className.c_str());
    });
    
    wrapper->SetUpdateCallback([](float dt) {
        scripting::interop::ScriptManager_OnUpdate(dt);
    });
    
    wrapper->SetDestroyCallback([entityId = entity.value]() {
        scripting::interop::ScriptManager_OnDestroy(entityId);
    });
    
    wrapper->SetCollisionEnterCallback([entityId = entity.value](uint64_t otherId) {
        scripting::interop::ScriptManager_OnCollisionEnter(entityId, otherId);
    });
    
    wrapper->SetCollisionExitCallback([entityId = entity.value](uint64_t otherId) {
        scripting::interop::ScriptManager_OnCollisionExit(entityId, otherId);
    });
    
    wrapper->SetTriggerEnterCallback([entityId = entity.value](uint64_t otherId) {
        scripting::interop::ScriptManager_OnTriggerEnter(entityId, otherId);
    });
    
    wrapper->SetTriggerExitCallback([entityId = entity.value](uint64_t otherId) {
        scripting::interop::ScriptManager_OnTriggerExit(entityId, otherId);
    });
    
    msc.instance = wrapper;
    msc.instance->OnCreate();
    
    GE_LOG_INFO("Managed script instantiated: %s on entity %llu", msc.className.c_str(), entity.value);
}

void ScriptSystem::ReloadScripts(World& world) {
    ScriptRegistry::ReloadAll(world);
}

void ScriptSystem::OnCollisionEnter(World& world, Entity entity, Entity other) {
    if (world.HasComponent<scripting::ManagedScriptComponent>(entity)) {
        auto& msc = world.GetComponent<scripting::ManagedScriptComponent>(entity);
        if (msc.instance) {
            msc.instance->OnCollisionEnter(other);
        }
    }
}

void ScriptSystem::OnCollisionExit(World& world, Entity entity, Entity other) {
    if (world.HasComponent<scripting::ManagedScriptComponent>(entity)) {
        auto& msc = world.GetComponent<scripting::ManagedScriptComponent>(entity);
        if (msc.instance) {
            msc.instance->OnCollisionExit(other);
        }
    }
}

void ScriptSystem::OnTriggerEnter(World& world, Entity entity, Entity other) {
    if (world.HasComponent<scripting::ManagedScriptComponent>(entity)) {
        auto& msc = world.GetComponent<scripting::ManagedScriptComponent>(entity);
        if (msc.instance) {
            msc.instance->OnTriggerEnter(other);
        }
    }
}

void ScriptSystem::OnTriggerExit(World& world, Entity entity, Entity other) {
    if (world.HasComponent<scripting::ManagedScriptComponent>(entity)) {
        auto& msc = world.GetComponent<scripting::ManagedScriptComponent>(entity);
        if (msc.instance) {
            msc.instance->OnTriggerExit(other);
        }
    }
}

} // namespace ecs
} // namespace ge