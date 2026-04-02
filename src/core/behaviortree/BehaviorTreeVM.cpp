#include "BehaviorTreeVM.h"

namespace ge {

BehaviorState BehaviorTreeVM::Execute(BehaviorTreeComponent& btComponent, void* context, float dt) {
    if (isPaused_ || btComponent.tree.nodes.empty()) {
        return BehaviorState::None;
    }
    
    context_.userContext = context;
    context_.deltaTime = dt;
    context_.timeElapsed += dt;
    
    BehaviorState result = ExecuteNode(btComponent.tree, btComponent.tree.rootNodeId, context_);
    
    lastState_ = result;
    return result;
}

void BehaviorTreeVM::Reset() {
    context_ = ExecutionContext();
    currentNodeId_ = 0;
    lastState_ = BehaviorState::None;
    isPaused_ = false;
}

BehaviorState BehaviorTreeVM::ExecuteNode(BehaviorTree& tree, uint64_t nodeId, ExecutionContext& ctx) {
    const BehaviorTreeNode* node = tree.FindNode(nodeId);
    if (!node) {
        return BehaviorState::Failure;
    }
    
    currentNodeId_ = nodeId;
    
    switch (node->type) {
        case BehaviorNodeType::Sequence:
        case BehaviorNodeType::Selector:
        case BehaviorNodeType::Parallel:
            return ExecuteComposite(*node, *node, ctx);
            
        case BehaviorNodeType::Decorator:
            return ExecuteDecorator(tree, *node, ctx);
            
        case BehaviorNodeType::Action:
            return ExecuteAction(tree, *node, ctx);
            
        case BehaviorNodeType::Condition:
            return ExecuteCondition(tree, *node, ctx);
            
        case BehaviorNodeType::SubTree:
            return BehaviorState::Failure;
            
        default:
            return BehaviorState::Failure;
    }
}

BehaviorState BehaviorTreeVM::ExecuteComposite(BehaviorTree& tree, const BehaviorTreeNode& node, ExecutionContext& ctx) {
    if (node.children.empty()) {
        return BehaviorState::Failure;
    }
    
    if (node.type == BehaviorNodeType::Selector) {
        for (uint64_t childId : node.children) {
            BehaviorState childState = ExecuteNode(tree, childId, ctx);
            if (childState == BehaviorState::Success) {
                return BehaviorState::Success;
            }
            if (childState == BehaviorState::Running) {
                return BehaviorState::Running;
            }
        }
        return BehaviorState::Failure;
    }
    
    if (node.type == BehaviorNodeType::Sequence) {
        for (uint64_t childId : node.children) {
            BehaviorState childState = ExecuteNode(tree, childId, ctx);
            if (childState == BehaviorState::Failure) {
                return BehaviorState::Failure;
            }
            if (childState == BehaviorState::Running) {
                return BehaviorState::Running;
            }
        }
        return BehaviorState::Success;
    }
    
    if (node.type == BehaviorNodeType::Parallel) {
        bool anyRunning = false;
        for (uint64_t childId : node.children) {
            BehaviorState childState = ExecuteNode(tree, childId, ctx);
            if (childState == BehaviorState::Running) {
                anyRunning = true;
            }
        }
        return anyRunning ? BehaviorState::Running : BehaviorState::Success;
    }
    
    return BehaviorState::Failure;
}

BehaviorState BehaviorTreeVM::ExecuteDecorator(BehaviorTree& tree, const BehaviorTreeNode& node, ExecutionContext& ctx) {
    if (node.children.empty()) {
        return BehaviorState::Failure;
    }
    
    uint64_t childId = node.children[0];
    BehaviorState childState = ExecuteNode(tree, childId, ctx);
    
    switch (node.decorator) {
        case DecoratorType::Inverter:
            if (childState == BehaviorState::Success) return BehaviorState::Failure;
            if (childState == BehaviorState::Failure) return BehaviorState::Success;
            return childState;
            
        case DecoratorType::Repeater: {
            float repeatCount = 1.0f;
            for (const auto& param : node.parameters) {
                if (param.first == "count") {
                    repeatCount = std::stof(param.second);
                }
            }
            auto& state = ctx.decoratorStates[node.id];
            state += 1.0f;
            if (state >= repeatCount) {
                state = 0.0f;
                return BehaviorState::Success;
            }
            return BehaviorState::Running;
        }
        
        case DecoratorType::UntilFail:
            if (childState == BehaviorState::Failure) {
                return BehaviorState::Success;
            }
            return BehaviorState::Running;
            
        case DecoratorType::UntilSuccess:
            if (childState == BehaviorState::Success) {
                return BehaviorState::Success;
            }
            return BehaviorState::Running;
            
        case DecoratorType::Cooldown: {
            float cooldownTime = 1.0f;
            for (const auto& param : node.parameters) {
                if (param.first == "time") {
                    cooldownTime = std::stof(param.second);
                }
            }
            auto& state = ctx.decoratorStates[node.id];
            if (state > 0.0f) {
                state -= ctx.deltaTime;
                return BehaviorState::Failure;
            }
            if (childState == BehaviorState::Success || childState == BehaviorState::Failure) {
                state = cooldownTime;
            }
            return childState;
        }
        
        case DecoratorType::Timeout: {
            float timeout = 5.0f;
            for (const auto& param : node.parameters) {
                if (param.first == "duration") {
                    timeout = std::stof(param.second);
                }
            }
            auto& state = ctx.decoratorStates[node.id];
            state += ctx.deltaTime;
            if (state >= timeout) {
                state = 0.0f;
                return BehaviorState::Failure;
            }
            if (childState == BehaviorState::Success || childState == BehaviorState::Failure) {
                state = 0.0f;
                return childState;
            }
            return BehaviorState::Running;
        }
        
        default:
            return childState;
    }
}

BehaviorState BehaviorTreeVM::ExecuteAction(BehaviorTree& tree, const BehaviorTreeNode& node, ExecutionContext& ctx) {
    BehaviorTreeRegistry& registry = BehaviorTreeRegistry::Get();
    
    for (const auto& param : node.parameters) {
        if (param.first == "action") {
            return registry.ExecuteAction(param.second, ctx.userContext);
        }
    }
    
    return BehaviorState::Failure;
}

BehaviorState BehaviorTreeVM::ExecuteCondition(BehaviorTree& tree, const BehaviorTreeNode& node, ExecutionContext& ctx) {
    BehaviorTreeRegistry& registry = BehaviorTreeRegistry::Get();
    
    for (const auto& param : node.parameters) {
        if (param.first == "condition") {
            return registry.ExecuteAction(param.second, ctx.userContext);
        }
    }
    
    return BehaviorState::Failure;
}

}
