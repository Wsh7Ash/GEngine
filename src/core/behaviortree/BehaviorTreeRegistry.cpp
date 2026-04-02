#include "BehaviorTreeRegistry.h"

namespace ge {

BehaviorTreeRegistry::BehaviorTreeRegistry() {
    nodeTypes_["Action"] = BehaviorNodeType::Action;
    nodeTypes_["Condition"] = BehaviorNodeType::Condition;
    nodeTypes_["Sequence"] = BehaviorNodeType::Sequence;
    nodeTypes_["Selector"] = BehaviorNodeType::Selector;
    nodeTypes_["Parallel"] = BehaviorNodeType::Parallel;
    nodeTypes_["Decorator"] = BehaviorNodeType::Decorator;
    nodeTypes_["SubTree"] = BehaviorNodeType::SubTree;
    
    typeNames_[BehaviorNodeType::Action] = "Action";
    typeNames_[BehaviorNodeType::Condition] = "Condition";
    typeNames_[BehaviorNodeType::Sequence] = "Sequence";
    typeNames_[BehaviorNodeType::Selector] = "Selector";
    typeNames_[BehaviorNodeType::Parallel] = "Parallel";
    typeNames_[BehaviorNodeType::Decorator] = "Decorator";
    typeNames_[BehaviorNodeType::SubTree] = "SubTree";
}

BehaviorTreeRegistry& BehaviorTreeRegistry::Get() {
    static BehaviorTreeRegistry instance;
    return instance;
}

void BehaviorTreeRegistry::RegisterNodeType(const std::string& typeName, BehaviorNodeType type) {
    nodeTypes_[typeName] = type;
    typeNames_[type] = typeName;
}

void BehaviorTreeRegistry::RegisterAction(const std::string& actionName, std::function<BehaviorState(void*)> action) {
    actions_[actionName] = action;
}

BehaviorNodeType BehaviorTreeRegistry::GetNodeType(const std::string& typeName) const {
    auto it = nodeTypes_.find(typeName);
    if (it != nodeTypes_.end()) {
        return it->second;
    }
    return BehaviorNodeType::Invalid;
}

const std::string& BehaviorTreeRegistry::GetNodeTypeName(BehaviorNodeType type) const {
    static std::string empty;
    auto it = typeNames_.find(type);
    if (it != typeNames_.end()) {
        return it->second;
    }
    return empty;
}

BehaviorState BehaviorTreeRegistry::ExecuteAction(const std::string& actionName, void* context) {
    auto it = actions_.find(actionName);
    if (it != actions_.end()) {
        return it->second(context);
    }
    return BehaviorState::Failure;
}

}
