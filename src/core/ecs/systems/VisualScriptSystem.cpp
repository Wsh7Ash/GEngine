#include "VisualScriptSystem.h"
#include "../../debug/log.h"
#include <algorithm>

namespace ge {
namespace ecs {

void VisualScriptSystem::Update(World& world, float ts) {
    for (auto const& entity : entities) {
        if (!world.HasComponent<visualscripting::VisualScriptComponent>(entity)) {
            continue;
        }

        auto& vsc = world.GetComponent<visualscripting::VisualScriptComponent>(entity);
        
        if (!vsc.isEnabled || !vsc.isInitialized) {
            continue;
        }

        visualscripting::VisualScriptExecutionContext ctx;
        ctx.entity = entity;
        ctx.world = &world;
        ctx.deltaTime = ts;
        ctx.elapsedTime = 0;
        ctx.localVariables = vsc.variables;

        auto* onUpdateNode = vsc.graph.FindNode("event_onupdate");
        if (onUpdateNode) {
            ExecuteFlowChain(world, entity, vsc, *onUpdateNode, ts);
        }

        vsc.variables = ctx.localVariables;
    }
}

void VisualScriptSystem::InitializeScripts(World& world) {
    auto entitiesWithScripts = world.Query<visualscripting::VisualScriptComponent>();
    
    for (auto entity : entitiesWithScripts) {
        auto& vsc = world.GetComponent<visualscripting::VisualScriptComponent>(entity);
        if (!vsc.isInitialized) {
            vsc.Initialize(entity, &world);
        }
    }
}

void VisualScriptSystem::TriggerEvent(World& world, Entity entity, const std::string& eventName) {
    if (!world.HasComponent<visualscripting::VisualScriptComponent>(entity)) {
        return;
    }

    auto& vsc = world.GetComponent<visualscripting::VisualScriptComponent>(entity);
    
    visualscripting::VisualScriptExecutionContext ctx;
    ctx.entity = entity;
    ctx.world = &world;
    ctx.deltaTime = 0;
    ctx.elapsedTime = 0;

    auto* eventNode = vsc.graph.FindNode(eventName);
    if (eventNode) {
        ExecuteFlowChain(world, entity, vsc, *eventNode, 0);
    }
}

void VisualScriptSystem::TriggerCollisionEnter(World& world, Entity entity, Entity other) {
    if (!world.HasComponent<visualscripting::VisualScriptComponent>(entity)) {
        return;
    }

    auto& vsc = world.GetComponent<visualscripting::VisualScriptComponent>(entity);
    
    visualscripting::VisualScriptExecutionContext ctx;
    ctx.entity = entity;
    ctx.world = &world;
    ctx.deltaTime = 0;
    ctx.elapsedTime = 0;
    ctx.localVariables["otherEntity"] = visualscripting::PinValue(other.value);

    auto* eventNode = vsc.graph.FindNode("event_oncollisionenter");
    if (eventNode) {
        ExecuteFlowChain(world, entity, vsc, *eventNode, 0);
    }
}

void VisualScriptSystem::TriggerCollisionExit(World& world, Entity entity, Entity other) {
    if (!world.HasComponent<visualscripting::VisualScriptComponent>(entity)) {
        return;
    }

    auto& vsc = world.GetComponent<visualscripting::VisualScriptComponent>(entity);
    
    visualscripting::VisualScriptExecutionContext ctx;
    ctx.entity = entity;
    ctx.world = &world;
    ctx.deltaTime = 0;
    ctx.elapsedTime = 0;
    ctx.localVariables["otherEntity"] = visualscripting::PinValue(other.value);

    auto* eventNode = vsc.graph.FindNode("event_oncollisionexit");
    if (eventNode) {
        ExecuteFlowChain(world, entity, vsc, *eventNode, 0);
    }
}

void VisualScriptSystem::ExecuteNode(World& world, Entity entity, 
                                     visualscripting::VisualScriptComponent& vsc,
                                     const visualscripting::VisualGraphNode& node, float ts) {
    const visualscripting::NodeDefinition* def = 
        visualscripting::NodeTypeRegistry::GetDefinition(node.definitionId);
    
    if (!def || !def->executeFunc) {
        return;
    }

    visualscripting::VisualScriptExecutionContext ctx;
    ctx.entity = entity;
    ctx.world = &world;
    ctx.deltaTime = ts;
    ctx.localVariables = vsc.variables;

    std::unordered_map<std::string, visualscripting::PinValue> outputs;
    def->executeFunc(ctx, node.inputValues, outputs);

    for (auto& [key, val] : outputs) {
        vsc.variables[key] = val;
    }
}

void VisualScriptSystem::ExecuteFlowChain(World& world, Entity entity,
                                          visualscripting::VisualScriptComponent& vsc,
                                          const visualscripting::VisualGraphNode& startNode, float ts) {
    ExecuteNode(world, entity, vsc, startNode, ts);

    auto flowIt = startNode.outputConnections.find("flow");
    if (flowIt != startNode.outputConnections.end()) {
        const visualscripting::VisualGraphNode* nextNode = vsc.graph.FindNode(flowIt->second);
        if (nextNode && nextNode->enabled) {
            ExecuteFlowChain(world, entity, vsc, *nextNode, ts);
        }
    }
}

} // namespace ecs
} // namespace ge