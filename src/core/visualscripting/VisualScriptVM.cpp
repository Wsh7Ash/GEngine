#include "VisualScriptVM.h"
#include "NodeTypeRegistry.h"
#include "../debug/log.h"

namespace ge {
namespace visualscripting {

CompiledScript VisualScriptCompiler::Compile(const VisualGraph& graph) {
    CompiledScript script;
    script.sourceGraphId = graph.id;

    for (const auto& var : graph.variables) {
        script.variableNames.push_back(var.name);
    }

    CompileEventNodes(graph, script);

    GE_LOG_INFO("VisualScriptCompiler: Compiled graph '%s' - %zu instructions", 
                graph.name.c_str(), script.instructions.size());
    return script;
}

void VisualScriptCompiler::CompileEventNodes(const VisualGraph& graph, CompiledScript& script) {
    for (const auto& node : graph.nodes) {
        if (node.definitionId.find("event_") == 0) {
            int nodeIdx = 0;
            for (int i = 0; i < (int)graph.nodes.size(); i++) {
                if (graph.nodes[i].id == node.id) {
                    nodeIdx = i;
                    break;
                }
            }
            CompileNode(graph, node, script, nodeIdx);
        }
    }
}

void VisualScriptCompiler::CompileNode(const VisualGraph& graph, const VisualGraphNode& node, 
                                       CompiledScript& script, int& instructionIndex) {
    (void)instructionIndex;

    const NodeDefinition* def = NodeTypeRegistry::GetDefinition(node.definitionId);
    if (!def) {
        GE_LOG_WARNING("VisualScriptCompiler: Unknown node type '%s'", node.definitionId.c_str());
        return;
    }

    int nodeStartIndex = (int)script.instructions.size();

    for (const auto& [pinId, value] : node.inputValues) {
        VMInstruction push;
        switch (value.type) {
            case PinType::Boolean:
                push.opcode = VMOpcode::PUSH_BOOL;
                break;
            case PinType::Integer:
                push.opcode = VMOpcode::PUSH_INT;
                break;
            case PinType::Float:
                push.opcode = VMOpcode::PUSH_FLOAT;
                break;
            case PinType::String:
                push.opcode = VMOpcode::PUSH_STRING;
                break;
            case PinType::Entity:
                push.opcode = VMOpcode::PUSH_ENTITY;
                break;
            default:
                continue;
        }
        push.operands.push_back(value);
        push.label = "push_" + pinId;
        script.instructions.push_back(push);
    }

    VMInstruction callNode;
    callNode.opcode = VMOpcode::CALL_NODE;
    callNode.stringOperand = node.definitionId;
    callNode.label = node.title;
    script.instructions.push_back(callNode);

    auto flowIt = node.outputConnections.find("flow");
    if (flowIt != node.outputConnections.end()) {
        const VisualGraphNode* targetNode = graph.FindNode(flowIt->second);
        if (targetNode) {
            int targetIdx = 0;
            for (int i = 0; i < (int)graph.nodes.size(); i++) {
                if (graph.nodes[i].id == targetNode->id) {
                    targetIdx = i;
                    break;
                }
            }
            VMInstruction jump;
            jump.opcode = VMOpcode::JUMP;
            jump.jumpTarget = targetIdx;
            script.instructions.push_back(jump);
        }
    }

    (void)nodeStartIndex;
}

bool VisualScriptCompiler::CompileToFile(const VisualGraph& graph, const std::string& outputPath) {
    CompiledScript script = Compile(graph);
    
    (void)outputPath;
    (void)script;
    
    GE_LOG_INFO("VisualScriptCompiler: Would compile to %s", outputPath.c_str());
    return true;
}

CompiledScript VisualScriptCompiler::LoadFromFile(const std::string& inputPath) {
    CompiledScript script;
    (void)inputPath;
    return script;
}

} // namespace visualscripting
} // namespace ge