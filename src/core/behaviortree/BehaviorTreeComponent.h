#pragma once

#include "BehaviorTreeNode.h"
#include "ecs/Entity.h"
#include <cstdint>
#include <vector>
#include <memory>

namespace ge {

struct BehaviorTreeComponent {
    BehaviorTree tree;
    uint64_t activeNodeId = 0;
    uint32_t tickCount = 0;
    float lastTickTime = 0.0f;
    bool isRunning = false;
    bool isPaused = false;
    
    BehaviorTreeComponent() = default;
    
    explicit BehaviorTreeComponent(const BehaviorTree& tree) 
        : tree(tree) {}
};

}
