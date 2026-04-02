#pragma once

#include "../System.h"
#include "../World.h"
#include "../../visualscripting/VisualScriptComponent.h"
#include "../../visualscripting/NodeTypeRegistry.h"

namespace ge {
namespace ecs {

class VisualScriptSystem : public System {
public:
    void Update(World& world, float ts);
    void InitializeScripts(World& world);
    
    void TriggerEvent(World& world, Entity entity, const std::string& eventName);
    void TriggerCollisionEnter(World& world, Entity entity, Entity other);
    void TriggerCollisionExit(World& world, Entity entity, Entity other);

private:
    void ExecuteNode(World& world, Entity entity, visualscripting::VisualScriptComponent& vsc, 
                     const visualscripting::VisualGraphNode& node, float ts);
    void ExecuteFlowChain(World& world, Entity entity, visualscripting::VisualScriptComponent& vsc,
                         const visualscripting::VisualGraphNode& startNode, float ts);
};

} // namespace ecs
} // namespace ge