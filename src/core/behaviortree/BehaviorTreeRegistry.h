#pragma once

#include "BehaviorTreeNode.h"
#include <unordered_map>
#include <string>
#include <memory>

namespace ge {

class BehaviorTreeRegistry {
public:
    static BehaviorTreeRegistry& Get();
    
    void RegisterNodeType(const std::string& typeName, BehaviorNodeType type);
    void RegisterAction(const std::string& actionName, std::function<BehaviorState(void*)> action);
    
    BehaviorNodeType GetNodeType(const std::string& typeName) const;
    const std::string& GetNodeTypeName(BehaviorNodeType type) const;
    
    std::unordered_map<std::string, BehaviorNodeType>& GetNodeTypes() { return nodeTypes_; }
    const std::unordered_map<std::string, BehaviorNodeType>& GetNodeTypes() const { return nodeTypes_; }
    
    BehaviorState ExecuteAction(const std::string& actionName, void* context);
    
private:
    BehaviorTreeRegistry();
    
    std::unordered_map<std::string, BehaviorNodeType> nodeTypes_;
    std::unordered_map<std::string, std::function<BehaviorState(void*)>> actions_;
    std::unordered_map<BehaviorNodeType, std::string> typeNames_;
};

}
