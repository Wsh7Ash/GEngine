#pragma once

#include "VisualGraphNode.h"
#include "VisualGraph.h"
#include <unordered_map>
#include <functional>
#include <string>

namespace ge {
namespace visualscripting {

class NodeTypeRegistry {
public:
    static void RegisterBuiltInNodes();
    static const NodeDefinition* GetDefinition(const std::string& id);
    static std::vector<NodeDefinition> GetDefinitionsByCategory(const std::string& category);
    static std::vector<std::string> GetAllCategories();
    static void Clear();
    static std::unordered_map<std::string, NodeDefinition>& GetRegistry();
};

} // namespace visualscripting
} // namespace ge