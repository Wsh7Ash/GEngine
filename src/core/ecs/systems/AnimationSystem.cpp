#include "AnimationSystem.h"
#include "../components/AnimatorComponent.h"
#include "../components/SpriteComponent.h"
#include "../components/ModelComponent.h"
#include "../../renderer/Model.h"

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
      sprite.isAnimated = false; // Disable legacy auto-looping in RenderSystem if set
    }

    // ── 3D Skeletal Animation Logic ──────────────────────────────
    if (animator.Is3D && world.HasComponent<ModelComponent>(entity)) {
        auto &modelComp = world.GetComponent<ModelComponent>(entity);
        if (modelComp.ModelPtr && !animator.SkeletalAnimations.empty()) {
            
            // Auto-populate from model if empty
            if (animator.SkeletalAnimations.empty()) {
                animator.SkeletalAnimations = modelComp.ModelPtr->GetAnimations();
            }

            if (animator.SkeletalAnimations.count(animator.CurrentState)) {
                auto &sa = animator.SkeletalAnimations[animator.CurrentState];
                animator.StateTime += dt;
                
                float ticksPerSecond = (float)sa.ticksPerSecond;
                if(ticksPerSecond == 0.0f) ticksPerSecond = 24.0f; 
                float timeInTicks = animator.StateTime * ticksPerSecond;
                float animationTime = fmod(timeInTicks, sa.duration);

                auto calculateBoneTransform = [&](auto& self, const AnimatorComponent::SkeletalAnimation::Node& node, const Math::Mat4& parentTransform) -> void {
                    std::string nodeName = node.name;
                    Math::Mat4 nodeTransform = node.transformation;

                    if (sa.tracks.count(nodeName)) {
                        auto &track = sa.tracks[nodeName];
                        
                        // --- Interpolate Position ---
                        Math::Vec3f pos = {0,0,0};
                        if(track.positions.size() == 1) pos = track.positions[0].position;
                        else {
                            for(size_t i=0; i<track.positions.size()-1; i++) {
                                if(animationTime < track.positions[i+1].timeStamp) {
                                    float t = (animationTime - track.positions[i].timeStamp) / (track.positions[i+1].timeStamp - track.positions[i].timeStamp);
                                    pos = Math::Lerp(track.positions[i].position, track.positions[i+1].position, t);
                                    break;
                                }
                            }
                        }

                        // --- Interpolate Rotation ---
                        Math::Quatf rot = Math::Quatf::Identity();
                        if(track.rotations.size() == 1) rot = track.rotations[0].orientation;
                        else {
                            for(size_t i=0; i<track.rotations.size()-1; i++) {
                                if(animationTime < track.rotations[i+1].timeStamp) {
                                    float t = (animationTime - track.rotations[i].timeStamp) / (track.rotations[i+1].timeStamp - track.rotations[i].timeStamp);
                                    rot = Math::Quatf::Slerp(track.rotations[i].orientation, track.rotations[i+1].orientation, t);
                                    break;
                                }
                            }
                        }

                        // --- Interpolate Scale ---
                        Math::Vec3f scale = {1,1,1};
                        if(track.scales.size() == 1) scale = track.scales[0].scale;
                        else {
                            for(size_t i=0; i<track.scales.size()-1; i++) {
                                if(animationTime < track.scales[i+1].timeStamp) {
                                    float t = (animationTime - track.scales[i].timeStamp) / (track.scales[i+1].timeStamp - track.scales[i].timeStamp);
                                    scale = Math::Lerp(track.scales[i].scale, track.scales[i+1].scale, t);
                                    break;
                                }
                            }
                        }

                        nodeTransform = Math::Mat4::TRS(pos, rot, scale);
                    }

                    Math::Mat4 globalTransform = parentTransform * nodeTransform;

                    if (sa.boneInfoMap.count(nodeName)) {
                        int index = sa.boneInfoMap[nodeName].id;
                        Math::Mat4 offset = sa.boneInfoMap[nodeName].offset;
                        if(index < animator.FinalBoneMatrices.size()) {
                            animator.FinalBoneMatrices[index] = globalTransform * offset;
                        }
                    }

                    for (const auto& child : node.children)
                        self(self, child, globalTransform);
                };

                calculateBoneTransform(calculateBoneTransform, sa.rootNode, Math::Mat4::Identity());
            }
        }
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
