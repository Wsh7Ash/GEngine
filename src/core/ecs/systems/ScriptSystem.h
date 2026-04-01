#pragma once

#include "../System.h"
#include "../../scripting/ManagedScriptComponent.h"

namespace ge {
namespace ecs {

    class ScriptSystem : public System
    {
    public:
        void Update(World& world, float ts);
        void ReloadScripts(World& world);
        void UpdateManagedScripts(World& world, float ts);

    private:
        void InstantiateManagedScript(World& world, Entity entity, scripting::ManagedScriptComponent& msc);
    };

} // namespace ecs
} // namespace ge
