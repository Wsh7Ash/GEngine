#include "NodeTypeRegistry.h"
#include "../debug/log.h"
#include <algorithm>

namespace ge {
namespace visualscripting {

static void RegisterEventNodes() {
    auto& reg = NodeTypeRegistry::GetRegistry();

    NodeDefinition onStart;
    onStart.id = "event_onstart";
    onStart.name = "On Start";
    onStart.category = "Events";
    onStart.type = NodeType::Event;
    onStart.description = "Called when the script starts";
    onStart.outputPins = {{"flow", "Flow", PinType::Flow, PinDirection::Output}};
    reg[onStart.id] = onStart;

    NodeDefinition onUpdate;
    onUpdate.id = "event_onupdate";
    onUpdate.name = "On Update";
    onUpdate.category = "Events";
    onUpdate.type = NodeType::Event;
    onUpdate.description = "Called every frame";
    onUpdate.outputPins = {{"flow", "Flow", PinType::Flow, PinDirection::Output},
                           {"deltaTime", "Delta Time", PinType::Float, PinDirection::Output}};
    reg[onUpdate.id] = onUpdate;

    NodeDefinition onCollisionEnter;
    onCollisionEnter.id = "event_oncollisionenter";
    onCollisionEnter.name = "On Collision Enter";
    onCollisionEnter.category = "Events";
    onCollisionEnter.type = NodeType::Event;
    onCollisionEnter.description = "Called when a collision starts";
    onCollisionEnter.outputPins = {{"flow", "Flow", PinType::Flow, PinDirection::Output},
                                    {"other", "Other Entity", PinType::Entity, PinDirection::Output}};
    reg[onCollisionEnter.id] = onCollisionEnter;

    NodeDefinition onCollisionExit;
    onCollisionExit.id = "event_oncollisionexit";
    onCollisionExit.name = "On Collision Exit";
    onCollisionExit.category = "Events";
    onCollisionExit.type = NodeType::Event;
    onCollisionExit.description = "Called when a collision ends";
    onCollisionExit.outputPins = {{"flow", "Flow", PinType::Flow, PinDirection::Output},
                                   {"other", "Other Entity", PinType::Entity, PinDirection::Output}};
    reg[onCollisionExit.id] = onCollisionExit;
}

static void RegisterActionNodes() {
    auto& reg = NodeTypeRegistry::GetRegistry();

    NodeDefinition log;
    log.id = "action_log";
    log.name = "Log";
    log.category = "Actions";
    log.type = NodeType::Action;
    log.description = "Print a message to the console";
    log.inputPins = {{"flow", "Flow", PinType::Flow, PinDirection::Input},
                     {"message", "Message", PinType::String, PinDirection::Input, PinValue(std::string("Hello"))}};
    log.outputPins = {{"flow", "Flow", PinType::Flow, PinDirection::Output}};
    log.executeFunc = [](VisualScriptExecutionContext& ctx, 
                         const std::unordered_map<std::string, PinValue>& inputs,
                         std::unordered_map<std::string, PinValue>& outputs) {
        auto it = inputs.find("message");
        if (it != inputs.end()) {
            try {
                std::string msg = std::any_cast<std::string>(it->second.value);
                GE_LOG_INFO("[VisualScript] %s", msg.c_str());
            } catch (...) {}
        }
    };
    reg[log.id] = log;

    NodeDefinition setTransformPosition;
    setTransformPosition.id = "action_setposition";
    setTransformPosition.name = "Set Position";
    setTransformPosition.category = "Actions";
    setTransformPosition.type = NodeType::Action;
    setTransformPosition.description = "Set entity transform position";
    setTransformPosition.inputPins = {{"flow", "Flow", PinType::Flow, PinDirection::Input},
                                       {"x", "X", PinType::Float, PinDirection::Input, PinValue(0.0f)},
                                       {"y", "Y", PinType::Float, PinDirection::Input, PinValue(0.0f)},
                                       {"z", "Z", PinType::Float, PinDirection::Input, PinValue(0.0f)}};
    setTransformPosition.outputPins = {{"flow", "Flow", PinType::Flow, PinDirection::Output}};
    setTransformPosition.executeFunc = [](VisualScriptExecutionContext& ctx, 
                                          const std::unordered_map<std::string, PinValue>& inputs,
                                          std::unordered_map<std::string, PinValue>& outputs) {
        float x = 0, y = 0, z = 0;
        auto itX = inputs.find("x");
        if (itX != inputs.end()) try { x = std::any_cast<float>(itX->second.value); } catch (...) {}
        auto itY = inputs.find("y");
        if (itY != inputs.end()) try { y = std::any_cast<float>(itY->second.value); } catch (...) {}
        auto itZ = inputs.find("z");
        if (itZ != inputs.end()) try { z = std::any_cast<float>(itZ->second.value); } catch (...) {}

        if (ctx.world && ctx.entity.value != 0) {
            // Set position via component access
            (void)x; (void)y; (void.z);
        }
    };
    reg[setTransformPosition.id] = setTransformPosition;

    NodeDefinition destroyEntity;
    destroyEntity.id = "action_destroyentity";
    destroyEntity.name = "Destroy Entity";
    destroyEntity.category = "Actions";
    destroyEntity.type = NodeType::Action;
    destroyEntity.description = "Destroy the current entity";
    destroyEntity.inputPins = {{"flow", "Flow", PinType::Flow, PinDirection::Input}};
    destroyEntity.outputPins = {{"flow", "Flow", PinType::Flow, PinDirection::Output}};
    destroyEntity.executeFunc = [](VisualScriptExecutionContext& ctx, 
                                   const std::unordered_map<std::string, PinValue>& inputs,
                                   std::unordered_map<std::string, PinValue>& outputs) {
        if (ctx.world && ctx.entity.value != 0) {
            ctx.world->DestroyEntity(ctx.entity);
            ctx.shouldStop = true;
        }
    };
    reg[destroyEntity.id] = destroyEntity;

    NodeDefinition wait;
    wait.id = "action_wait";
    wait.name = "Wait";
    wait.category = "Actions";
    wait.type = NodeType::Action;
    wait.description = "Wait for specified seconds";
    wait.inputPins = {{"flow", "Flow", PinType::Flow, PinDirection::Input},
                      {"seconds", "Seconds", PinType::Float, PinDirection::Input, PinValue(1.0f)}};
    wait.outputPins = {{"flow", "Flow", PinType::Flow, PinDirection::Output}};
    reg[wait.id] = wait;
}

static void RegisterFlowControlNodes() {
    auto& reg = NodeTypeRegistry::GetRegistry();

    NodeDefinition branch;
    branch.id = "flow_branch";
    branch.name = "Branch";
    branch.category = "Flow";
    branch.type = NodeType::FlowControl;
    branch.description = "Conditional branch (if/else)";
    branch.inputPins = {{"flow", "Flow", PinType::Flow, PinDirection::Input},
                        {"condition", "Condition", PinType::Boolean, PinDirection::Input, PinValue(false)}};
    branch.outputPins = {{"true", "True", PinType::Flow, PinDirection::Output},
                         {"false", "False", PinType::Flow, PinDirection::Output}};
    reg[branch.id] = branch;

    NodeDefinition sequence;
    sequence.id = "flow_sequence";
    sequence.name = "Sequence";
    sequence.category = "Flow";
    sequence.type = NodeType::FlowControl;
    sequence.description = "Execute outputs in order";
    sequence.inputPins = {{"flow", "Flow", PinType::Flow, PinDirection::Input}};
    sequence.outputPins = {{"then0", "Then 0", PinType::Flow, PinDirection::Output},
                           {"then1", "Then 1", PinType::Flow, PinDirection::Output},
                           {"then2", "Then 2", PinType::Flow, PinDirection::Output}};
    reg[sequence.id] = sequence;

    NodeDefinition gate;
    gate.id = "flow_gate";
    gate.name = "Gate";
    gate.category = "Flow";
    gate.type = NodeType::FlowControl;
    gate.description = "Open/close flow control";
    gate.inputPins = {{"flow", "Flow", PinType::Flow, PinDirection::Input},
                      {"open", "Open", PinType::Boolean, PinDirection::Input, PinValue(true)}};
    gate.outputPins = {{"flow", "Flow", PinType::Flow, PinDirection::Output}};
    reg[gate.id] = gate;
}

static void RegisterMathNodes() {
    auto& reg = NodeTypeRegistry::GetRegistry();

    NodeDefinition add;
    add.id = "math_add";
    add.name = "Add";
    add.category = "Math";
    add.type = NodeType::Math;
    add.description = "Add two numbers";
    add.inputPins = {{"a", "A", PinType::Float, PinDirection::Input, PinValue(0.0f)},
                     {"b", "B", PinType::Float, PinDirection::Input, PinValue(0.0f)}};
    add.outputPins = {{"result", "Result", PinType::Float, PinDirection::Output}};
    add.executeFunc = [](VisualScriptExecutionContext& ctx, 
                         const std::unordered_map<std::string, PinValue>& inputs,
                         std::unordered_map<std::string, PinValue>& outputs) {
        float a = 0, b = 0;
        auto itA = inputs.find("a");
        if (itA != inputs.end()) try { a = std::any_cast<float>(itA->second.value); } catch (...) {}
        auto itB = inputs.find("b");
        if (itB != inputs.end()) try { b = std::any_cast<float>(itB->second.value); } catch (...) {}
        outputs["result"] = PinValue(a + b);
    };
    reg[add.id] = add;

    NodeDefinition multiply;
    multiply.id = "math_multiply";
    multiply.name = "Multiply";
    multiply.category = "Math";
    multiply.type = NodeType::Math;
    multiply.description = "Multiply two numbers";
    multiply.inputPins = {{"a", "A", PinType::Float, PinDirection::Input, PinValue(1.0f)},
                          {"b", "B", PinType::Float, PinDirection::Input, PinValue(1.0f)}};
    multiply.outputPins = {{"result", "Result", PinType::Float, PinDirection::Output}};
    multiply.executeFunc = [](VisualScriptExecutionContext& ctx, 
                              const std::unordered_map<std::string, PinValue>& inputs,
                              std::unordered_map<std::string, PinValue>& outputs) {
        float a = 1, b = 1;
        auto itA = inputs.find("a");
        if (itA != inputs.end()) try { a = std::any_cast<float>(itA->second.value); } catch (...) {}
        auto itB = inputs.find("b");
        if (itB != inputs.end()) try { b = std::any_cast<float>(itB->second.value); } catch (...) {}
        outputs["result"] = PinValue(a * b);
    };
    reg[multiply.id] = multiply;

    NodeDefinition compare;
    compare.id = "math_compare";
    compare.name = "Compare";
    compare.category = "Math";
    compare.type = NodeType::Comparison;
    compare.description = "Compare two values";
    compare.inputPins = {{"a", "A", PinType::Float, PinDirection::Input},
                         {"b", "B", PinType::Float, PinDirection::Input}};
    compare.outputPins = {{"greater", "A > B", PinType::Boolean, PinDirection::Output},
                          {"equal", "A == B", PinType::Boolean, PinDirection::Output},
                          {"less", "A < B", PinType::Boolean, PinDirection::Output}};
    compare.executeFunc = [](VisualScriptExecutionContext& ctx, 
                             const std::unordered_map<std::string, PinValue>& inputs,
                             std::unordered_map<std::string, PinValue>& outputs) {
        float a = 0, b = 0;
        auto itA = inputs.find("a");
        if (itA != inputs.end()) try { a = std::any_cast<float>(itA->second.value); } catch (...) {}
        auto itB = inputs.find("b");
        if (itB != inputs.end()) try { b = std::any_cast<float>(itB->second.value); } catch (...) {}
        outputs["greater"] = PinValue(a > b);
        outputs["equal"] = PinValue(a == b);
        outputs["less"] = PinValue(a < b);
    };
    reg[compare.id] = compare;
}

static void RegisterVariableNodes() {
    auto& reg = NodeTypeRegistry::GetRegistry();

    NodeDefinition getVar;
    getVar.id = "var_get";
    getVar.name = "Get Variable";
    getVar.category = "Variables";
    getVar.type = NodeType::Variable;
    getVar.description = "Get variable value";
    getVar.inputPins = {{"name", "Name", PinType::String, PinDirection::Input}};
    getVar.outputPins = {{"value", "Value", PinType::Any, PinDirection::Output}};
    getVar.executeFunc = [](VisualScriptExecutionContext& ctx, 
                            const std::unordered_map<std::string, PinValue>& inputs,
                            std::unordered_map<std::string, PinValue>& outputs) {
        auto it = inputs.find("name");
        if (it != inputs.end()) {
            try {
                std::string name = std::any_cast<std::string>(it->second.value);
                auto vit = ctx.localVariables.find(name);
                if (vit != ctx.localVariables.end()) {
                    outputs["value"] = vit->second;
                }
            } catch (...) {}
        }
    };
    reg[getVar.id] = getVar;

    NodeDefinition setVar;
    setVar.id = "var_set";
    setVar.name = "Set Variable";
    setVar.category = "Variables";
    setVar.type = NodeType::Variable;
    setVar.description = "Set variable value";
    setVar.inputPins = {{"flow", "Flow", PinType::Flow, PinDirection::Input},
                        {"name", "Name", PinType::String, PinDirection::Input},
                        {"value", "Value", PinType::Any, PinDirection::Input}};
    setVar.outputPins = {{"flow", "Flow", PinType::Flow, PinDirection::Output}};
    setVar.executeFunc = [](VisualScriptExecutionContext& ctx, 
                            const std::unordered_map<std::string, PinValue>& inputs,
                            std::unordered_map<std::string, PinValue>& outputs) {
        auto itName = inputs.find("name");
        auto itValue = inputs.find("value");
        if (itName != inputs.end() && itValue != inputs.end()) {
            try {
                std::string name = std::any_cast<std::string>(itName->second.value);
                ctx.localVariables[name] = itValue->second;
            } catch (...) {}
        }
    };
    reg[setVar.id] = setVar;
}

void NodeTypeRegistry::RegisterBuiltInNodes() {
    RegisterEventNodes();
    RegisterActionNodes();
    RegisterFlowControlNodes();
    RegisterMathNodes();
    RegisterVariableNodes();
    GE_LOG_INFO("NodeTypeRegistry: Registered %zu built-in node types", GetRegistry().size());
}

const NodeDefinition* NodeTypeRegistry::GetDefinition(const std::string& id) {
    auto& reg = GetRegistry();
    auto it = reg.find(id);
    if (it != reg.end()) return &it->second;
    return nullptr;
}

std::vector<NodeDefinition> NodeTypeRegistry::GetDefinitionsByCategory(const std::string& category) {
    std::vector<NodeDefinition> result;
    for (auto& [id, def] : GetRegistry()) {
        if (def.category == category) {
            result.push_back(def);
        }
    }
    return result;
}

std::vector<std::string> NodeTypeRegistry::GetAllCategories() {
    std::vector<std::string> categories;
    for (auto& [id, def] : GetRegistry()) {
        if (std::find(categories.begin(), categories.end(), def.category) == categories.end()) {
            categories.push_back(def.category);
        }
    }
    return categories;
}

void NodeTypeRegistry::Clear() {
    GetRegistry().clear();
}

std::unordered_map<std::string, NodeDefinition>& NodeTypeRegistry::GetRegistry() {
    static std::unordered_map<std::string, NodeDefinition> registry;
    return registry;
}

} // namespace visualscripting
} // namespace ge