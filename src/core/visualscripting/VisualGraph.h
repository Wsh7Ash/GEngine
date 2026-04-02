#pragma once

#include "VisualGraphNode.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace ge {
namespace visualscripting {

struct VisualGraphConnection {
    std::string fromNodeId;
    std::string fromPinId;
    std::string toNodeId;
    std::string toPinId;
};

struct VisualGraphVariable {
    std::string name;
    PinType type = PinType::Float;
    PinValue initialValue;
};

struct VisualGraph {
    std::string id;
    std::string name;
    std::string description;
    std::string filePath;

    std::vector<VisualGraphNode> nodes;
    std::vector<VisualGraphConnection> connections;
    std::vector<VisualGraphVariable> variables;

    VisualGraphNode* FindNode(const std::string& nodeId) {
        for (auto& node : nodes) {
            if (node.id == nodeId) return &node;
        }
        return nullptr;
    }

    const VisualGraphNode* FindNode(const std::string& nodeId) const {
        for (auto& node : nodes) {
            if (node.id == nodeId) return &node;
        }
        return nullptr;
    }

    VisualGraphNode* FindEventNode(const std::string& eventName) {
        for (auto& node : nodes) {
            if (node.definitionId == eventName) return &node;
        }
        return nullptr;
    }
};

class VisualGraphSerializer {
public:
    static bool Save(const VisualGraph& graph, const std::string& filePath);
    static bool Load(VisualGraph& graph, const std::string& filePath);
    static std::string SerializeToString(const VisualGraph& graph);
    static bool DeserializeFromString(VisualGraph& graph, const std::string& json);
};

} // namespace visualscripting
} // namespace ge