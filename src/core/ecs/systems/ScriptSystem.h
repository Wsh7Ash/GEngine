#pragma once

#include "../System.h"
#include "../../scripting/ManagedScriptComponent.h"
#include "../World.h"

namespace ge {
namespace ecs {

    class ScriptSystem : public System
    {
    public:
        void Update(World& world, float ts);
        void ReloadScripts(World& world);
        void UpdateManagedScripts(World& world, float ts);
        
        void OnCollisionEnter(World& world, Entity entity, Entity other);
        void OnCollisionExit(World& world, Entity entity, Entity other);
        void OnTriggerEnter(World& world, Entity entity, Entity other);
        void OnTriggerExit(World& world, Entity entity, Entity other);

    private:
        void InstantiateManagedScript(World& world, Entity entity, scripting::ManagedScriptComponent& msc);
    };

} // namespace ecs
} // namespace ge
