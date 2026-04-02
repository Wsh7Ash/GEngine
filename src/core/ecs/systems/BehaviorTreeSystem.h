#pragma once

#include "../System.h"
#include "../World.h"
#include "../../behaviortree/BehaviorTreeComponent.h"
#include "../../behaviortree/BehaviorTreeVM.h"
#include <memory>

namespace ge {
namespace ecs {

class BehaviorTreeSystem : public System {
public:
    BehaviorTreeSystem();
    virtual ~BehaviorTreeSystem() = default;
    
    void Update(World& world, float ts);
    
    void SetContext(void* ctx) { context_ = ctx; }
    void* GetContext() const { return context_; }
    
    void Pause() { isPaused_ = true; }
    void Resume() { isPaused_ = false; }
    bool IsPaused() const { return isPaused_; }
    
private:
    void ExecuteBehaviorTree(World& world, Entity entity, BehaviorTreeComponent& bt, float ts);
    
    std::unique_ptr<BehaviorTreeVM> vm_;
    void* context_ = nullptr;
    bool isPaused_ = false;
};

} // namespace ecs
} // namespace ge
