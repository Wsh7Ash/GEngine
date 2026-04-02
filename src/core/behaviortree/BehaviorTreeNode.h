#pragma once

#include "../containers/handle.h"
#include <cstdint>
#include <vector>
#include <string>
#include <functional>
#include <memory>

namespace ge {

enum class BehaviorNodeType : uint8_t {
    Invalid = 0,
    Action,
    Condition,
    Sequence,
    Selector,
    Parallel,
    Decorator,
    SubTree
};

enum class BehaviorState : uint8_t {
    None = 0,
    Running,
    Success,
    Failure
};

enum class DecoratorType : uint8_t {
    None = 0,
    Inverter,
    Repeater,
    UntilFail,
    UntilSuccess,
    Cooldown,
    Timeout
};

struct BehaviorTreeNode {
    uint64_t id = 0;
    BehaviorNodeType type = BehaviorNodeType::Invalid;
    std::string name;
    std::vector<uint64_t> children;
    std::vector<std::pair<std::string, std::string>> parameters;
    DecoratorType decorator = DecoratorType::None;
    
    BehaviorTreeNode() = default;
    explicit BehaviorTreeNode(BehaviorNodeType type, const std::string& name) 
        : type(type), name(name) {}
};

struct BehaviorTree {
    uint64_t rootNodeId = 0;
    std::vector<BehaviorTreeNode> nodes;
    std::string name;
    uint32_t version = 1;
    
    BehaviorTreeNode* FindNode(uint64_t id) {
        for (auto& node : nodes) {
            if (node.id == id) return &node;
        }
        return nullptr;
    }
    
    const BehaviorTreeNode* FindNode(uint64_t id) const {
        for (const auto& node : nodes) {
            if (node.id == id) return &node;
        }
        return nullptr;
    }
    
    void AddNode(BehaviorTreeNode node) {
        nodes.push_back(node);
    }
    
    void RemoveNode(uint64_t id) {
        for (auto it = nodes.begin(); it != nodes.end(); ++it) {
            if (it->id == id) {
                for (auto& n : nodes) {
                    for (auto cit = n.children.begin(); cit != n.children.end(); ++cit) {
                        if (*cit == id) {
                            cit = n.children.erase(cit);
                        }
                    }
                }
                nodes.erase(it);
                return;
            }
        }
    }
};

class BehaviorNodeExecutor {
public:
    virtual ~BehaviorNodeExecutor() = default;
    virtual BehaviorState Execute(const BehaviorTreeNode& node, void* context) = 0;
    virtual void Reset() {}
};

using NodeExecutorFactory = std::function<std::shared_ptr<BehaviorNodeExecutor>()>;

}
