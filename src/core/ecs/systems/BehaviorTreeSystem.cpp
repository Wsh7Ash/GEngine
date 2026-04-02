#include "BehaviorTreeSystem.h"
#include "World.h"
#include "../../behaviortree/BehaviorTreeComponent.h"
#include "../../behaviortree/BehaviorTreeVM.h"

namespace ge {
namespace ecs {

BehaviorTreeSystem::BehaviorTreeSystem() {
    vm_ = std::make_unique<BehaviorTreeVM>();
}

void BehaviorTreeSystem::Update(World& world, float ts) {
    if (isPaused_) {
        return;
    }
    
    for (Entity entity : entities) {
        if (world.HasComponent<BehaviorTreeComponent>(entity)) {
            BehaviorTreeComponent& bt = world.GetComponent<BehaviorTreeComponent>(entity);
            ExecuteBehaviorTree(world, entity, bt, ts);
        }
    }
}

void BehaviorTreeSystem::ExecuteBehaviorTree(World& world, Entity entity, BehaviorTreeComponent& bt, float ts) {
    if (bt.tree.nodes.empty()) {
        return;
    }
    
    context_ = &world;
    BehaviorState state = vm_->Execute(bt, context_, ts);
    
    bt.isRunning = (state == BehaviorState::Running);
    bt.lastTickTime = ts;
    bt.tickCount++;
    
    if (bt.activeNodeId != vm_->GetCurrentNodeId()) {
        bt.activeNodeId = vm_->GetCurrentNodeId();
    }
}

}
}
