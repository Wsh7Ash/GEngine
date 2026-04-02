#pragma once

#include "BehaviorTreeNode.h"
#include "BehaviorTreeRegistry.h"
#include "BehaviorTreeComponent.h"
#include <stack>
#include <unordered_map>
#include <vector>

namespace ge {

struct ExecutionContext {
    void* userContext = nullptr;
    float deltaTime = 0.0f;
    float timeElapsed = 0.0f;
    std::stack<uint64_t> nodeStack;
    std::unordered_map<uint64_t, BehaviorState> nodeStates;
    std::unordered_map<uint64_t, float> decoratorStates;
};

class BehaviorTreeVM {
public:
    BehaviorTreeVM() = default;
    
    BehaviorState Execute(BehaviorTreeComponent& btComponent, void* context, float dt);
    
    void Reset();
    void Pause() { isPaused_ = true; }
    void Resume() { isPaused_ = false; }
    bool IsPaused() const { return isPaused_; }
    
    uint64_t GetCurrentNodeId() const { return currentNodeId_; }
    BehaviorState GetLastState() const { return lastState_; }
    
private:
    BehaviorState ExecuteNode(BehaviorTree& tree, uint64_t nodeId, ExecutionContext& ctx);
    BehaviorState ExecuteComposite(BehaviorTree& tree, const BehaviorTreeNode& node, ExecutionContext& ctx);
    BehaviorState ExecuteDecorator(BehaviorTree& tree, const BehaviorTreeNode& node, ExecutionContext& ctx);
    BehaviorState ExecuteAction(BehaviorTree& tree, const BehaviorTreeNode& node, ExecutionContext& ctx);
    BehaviorState ExecuteCondition(BehaviorTree& tree, const BehaviorTreeNode& node, ExecutionContext& ctx);
    
    bool isPaused_ = false;
    uint64_t currentNodeId_ = 0;
    BehaviorState lastState_ = BehaviorState::None;
    ExecutionContext context_;
};

}
