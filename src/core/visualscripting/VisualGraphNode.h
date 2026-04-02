#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <any>
#include <cstdint>

namespace ge {
namespace visualscripting {

enum class PinType {
    None,
    Flow,
    Boolean,
    Integer,
    Float,
    String,
    Vector3,
    Entity,
    Any
};

enum class PinDirection {
    Input,
    Output
};

struct PinValue {
    PinType type = PinType::None;
    std::any value;

    PinValue() = default;
    explicit PinValue(bool v) : type(PinType::Boolean), value(v) {}
    explicit PinValue(int v) : type(PinType::Integer), value(v) {}
    explicit PinValue(float v) : type(PinType::Float), value(v) {}
    explicit PinValue(const std::string& v) : type(PinType::String), value(v) {}
    explicit PinValue(uint64_t v) : type(PinType::Entity), value(v) {}
};

struct Pin {
    std::string id;
    std::string label;
    PinType type = PinType::None;
    PinDirection direction = PinDirection::Input;
    PinValue defaultValue;
    int nodeIndex = -1;

    bool IsConnected() const { return !connections.empty(); }
    std::vector<std::pair<int, std::string>> connections;
};

enum class NodeType {
    Event,
    Action,
    FlowControl,
    Variable,
    Math,
    Comparison,
    Entity,
    Component,
    Custom
};

struct NodeDefinition {
    std::string id;
    std::string name;
    std::string category;
    NodeType type = NodeType::Action;
    std::string description;
    std::vector<Pin> inputPins;
    std::vector<Pin> outputPins;

    using ExecuteFunc = std::function<void(class VisualScriptExecutionContext&, const std::unordered_map<std::string, PinValue>&, std::unordered_map<std::string, PinValue>&)>;
    ExecuteFunc executeFunc;
};

struct VisualGraphNode {
    std::string id;
    std::string definitionId;
    std::string title;
    float posX = 0, posY = 0;
    bool enabled = true;

    std::unordered_map<std::string, PinValue> inputValues;
    std::unordered_map<std::string, PinValue> outputValues;

    std::unordered_map<std::string, std::string> inputConnections;
    std::unordered_map<std::string, std::string> outputConnections;

    void SetInput(const std::string& pinId, bool value) { inputValues[pinId] = PinValue(value); }
    void SetInput(const std::string& pinId, int value) { inputValues[pinId] = PinValue(value); }
    void SetInput(const std::string& pinId, float value) { inputValues[pinId] = PinValue(value); }
    void SetInput(const std::string& pinId, const std::string& value) { inputValues[pinId] = PinValue(value); }
    void SetInput(const std::string& pinId, uint64_t value) { inputValues[pinId] = PinValue(value); }

    template<typename T>
    T GetInput(const std::string& pinId, const T& defaultVal) const {
        auto it = inputValues.find(pinId);
        if (it != inputValues.end()) {
            try { return std::any_cast<T>(it->second.value); }
            catch (...) { return defaultVal; }
        }
        return defaultVal;
    }

    void SetOutput(const std::string& pinId, bool value) { outputValues[pinId] = PinValue(value); }
    void SetOutput(const std::string& pinId, int value) { outputValues[pinId] = PinValue(value); }
    void SetOutput(const std::string& pinId, float value) { outputValues[pinId] = PinValue(value); }
    void SetOutput(const std::string& pinId, const std::string& value) { outputValues[pinId] = PinValue(value); }
    void SetOutput(const std::string& pinId, uint64_t value) { outputValues[pinId] = PinValue(value); }

    template<typename T>
    T GetOutput(const std::string& pinId, const T& defaultVal) const {
        auto it = outputValues.find(pinId);
        if (it != outputValues.end()) {
            try { return std::any_cast<T>(it->second.value); }
            catch (...) { return defaultVal; }
        }
        return defaultVal;
    }
};

} // namespace visualscripting
} // namespace ge