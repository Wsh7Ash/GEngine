#include "ScriptSystem.h"
#include "../components/NativeScriptComponent.h"
#include "../World.h"

namespace ge {
namespace ecs {

    void ScriptSystem::Update(World& world, float ts)
    {
        for (auto const& entity : entities)
        {
            auto& nsc = world.GetComponent<NativeScriptComponent>(entity);

            // 1. Instantiate if needed
            if (!nsc.instance)
            {
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
