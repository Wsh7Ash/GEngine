#include "AnimationSystem.h"
#include "../components/AnimatorComponent.h"
#include "../components/SpriteComponent.h"
#include "../components/SpriteComponent.h"

namespace ge {
namespace ecs {

void AnimationSystem::Update(World &world, float dt) {
  for (auto const &entity : entities) {
    if (!world.HasComponent<AnimatorComponent>(entity) ||
        !world.HasComponent<SpriteComponent>(entity))
      continue;

    auto &animator = world.GetComponent<AnimatorComponent>(entity);
    auto &sprite = world.GetComponent<SpriteComponent>(entity);

    if (animator.CurrentState.empty() || animator.Animations.empty())
        continue;

    // 1. Evaluate Transitions
    if (animator.Transitions.count(animator.CurrentState)) {
      for (const auto &transition : animator.Transitions[animator.CurrentState]) {
        if (EvaluateTransition(animator, transition)) {
          animator.CurrentState = transition.TargetState;
          animator.StateTime = 0.0f;
          animator.CurrentFrameIndex = 0;
          break; // Only one transition per frame
        }
      }
    }

    // 2. Update Animation
    if (animator.Animations.count(animator.CurrentState)) {
      auto &animation = animator.Animations[animator.CurrentState];
      if (animation.Frames.empty()) continue;

      animator.StateTime += dt;

      // Find current frame based on elapsed time in state
      // For simplicity, we assume fixed frame rate or sum up durations
      float totalDuration = 0.0f;
      for (const auto& frame : animation.Frames) totalDuration += frame.Duration;

      if (animation.Loop && totalDuration > 0.0f) {
          while (animator.StateTime >= totalDuration) {
              animator.StateTime -= totalDuration;
          }
      }

      float accumulatedTime = 0.0f;
      int frameIdx = 0;
      for (size_t i = 0; i < animation.Frames.size(); ++i) {
          accumulatedTime += animation.Frames[i].Duration;
          if (animator.StateTime <= accumulatedTime) {
              frameIdx = (int)i;
              break;
          }
          if (i == animation.Frames.size() - 1) { // Last frame
              frameIdx = (int)i;
          }
      }

      animator.CurrentFrameIndex = frameIdx;
      
      // Sync with SpriteComponent
      sprite.currentFrame = animation.Frames[frameIdx].Index;
      sprite.isAnimated = false; // Disable legacy auto-looping in RenderSystem if set
    }
  }
}

bool AnimationSystem::EvaluateTransition(const AnimatorComponent &animator,
                                         const AnimationTransition &transition) {
  if (transition.ConditionParameter.empty()) return true; // Direct transition

  // Float Params
  if (animator.FloatParams.count(transition.ConditionParameter)) {
    float val = animator.FloatParams.at(transition.ConditionParameter);
    switch (transition.Op) {
    case AnimationTransition::ConditionOperator::Greater:  return val > transition.Threshold;
    case AnimationTransition::ConditionOperator::Less:     return val < transition.Threshold;
    case AnimationTransition::ConditionOperator::Equal:    return val == transition.Threshold;
    case AnimationTransition::ConditionOperator::NotEqual: return val != transition.Threshold;
    default: break;
    }
  }

  // Bool Params
  if (animator.BoolParams.count(transition.ConditionParameter)) {
    bool val = animator.BoolParams.at(transition.ConditionParameter);
    switch (transition.Op) {
    case AnimationTransition::ConditionOperator::IsTrue:  return val == true;
    case AnimationTransition::ConditionOperator::IsFalse: return val == false;
    default: break;
    }
  }

  return false;
}

} // namespace ecs
} // namespace ge
