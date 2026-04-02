#pragma once

#include "../ecs/Entity.h"
#include "../ecs/World.h"
#include "VisualGraphNode.h"
#include <string>
#include <vector>

namespace ge {
namespace visualscripting {

struct VisualScriptExecutionContext {
    ecs::Entity entity;
    ecs::World* world;
    float deltaTime;
    float elapsedTime;
    std::unordered_map<std::string, PinValue> localVariables;
    std::unordered_map<std::string, PinValue> globalVariables;
    bool shouldStop = false;
    int currentFlowNodeIndex = -1;
};

struct VisualScriptComponent {
    VisualScriptComponent() = default;

    ~VisualScriptComponent() {
        variables.clear();
    }

    VisualScriptComponent(const VisualScriptComponent&) = delete;
    VisualScriptComponent& operator=(const VisualScriptComponent&) = delete;

    VisualScriptComponent(VisualScriptComponent&& other) noexcept
        : graphPath(std::move(other.graphPath)),
          graph(std::move(other.graph)),
          variables(std::move(other.variables)),
          isInitialized(other.isInitialized),
          isEnabled(other.isEnabled) {
        other.isInitialized = false;
    }

    VisualScriptComponent& operator=(VisualScriptComponent&& other) noexcept {
        if (this != &other) {
            graphPath = std::move(other.graphPath);
            graph = std::move(other.graph);
            variables = std::move(other.variables);
            isInitialized = other.isInitialized;
            isEnabled = other.isEnabled;
            other.isInitialized = false;
        }
        return *this;
    }

    std::string graphPath;
    VisualGraph graph;
    std::unordered_map<std::string, PinValue> variables;
    bool isInitialized = false;
    bool isEnabled = true;

    void Initialize(ecs::Entity entity, ecs::World* world);
    void ExecuteEvent(const std::string& eventName, VisualScriptExecutionContext& ctx);
    void Update(VisualScriptExecutionContext& ctx);
    void Shutdown();

    void SetVariable(const std::string& name, bool value) { variables[name] = PinValue(value); }
    void SetVariable(const std::string& name, int value) { variables[name] = PinValue(value); }
    void SetVariable(const std::string& name, float value) { variables[name] = PinValue(value); }
    void SetVariable(const std::string& name, const std::string& value) { variables[name] = PinValue(value); }
    void SetVariable(const std::string& name, uint64_t value) { variables[name] = PinValue(value); }

    template<typename T>
    T GetVariable(const std::string& name, const T& defaultVal) const {
        auto it = variables.find(name);
        if (it != variables.end()) {
            try { return std::any_cast<T>(it->second.value); }
            catch (...) { return defaultVal; }
        }
        return defaultVal;
    }
};

} // namespace visualscripting
} // namespace ge