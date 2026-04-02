#pragma once

#include "VisualGraphNode.h"
#include "VisualGraph.h"
#include <vector>
#include <string>

namespace ge {
namespace visualscripting {

enum class VMOpcode : uint8_t {
    NOP = 0,
    PUSH_BOOL,
    PUSH_INT,
    PUSH_FLOAT,
    PUSH_STRING,
    PUSH_ENTITY,
    POP,
    STORE_VAR,
    LOAD_VAR,
    ADD,
    SUB,
    MUL,
    DIV,
    GT,
    LT,
    EQ,
    AND,
    OR,
    NOT,
    CALL_NODE,
    JUMP,
    JUMP_IF_TRUE,
    JUMP_IF_FALSE,
    RETURN,
    LOG,
    SET_POSITION,
    DESTROY_ENTITY,
    GET_DELTA_TIME,
    GET_ENTITY_ID,
    HAS_COMPONENT,
    GET_COMPONENT
};

struct VMInstruction {
    VMOpcode opcode;
    std::string label;
    std::vector<PinValue> operands;
    int jumpTarget = -1;
    std::string stringOperand;
};

struct CompiledScript {
    std::vector<VMInstruction> instructions;
    std::vector<std::string> variableNames;
    std::string sourceGraphId;
};

class VisualScriptCompiler {
public:
    static CompiledScript Compile(const VisualGraph& graph);
    static bool CompileToFile(const VisualGraph& graph, const std::string& outputPath);
    static CompiledScript LoadFromFile(const std::string& inputPath);

private:
    static void CompileNode(const VisualGraph& graph, const VisualGraphNode& node, 
                           CompiledScript& script, int& instructionIndex);
    static void CompileEventNodes(const VisualGraph& graph, CompiledScript& script);
};

} // namespace visualscripting
} // namespace ge