#include "AnimationSystem.h"
#include "../components/AnimatorComponent.h"
#include "../components/SpriteComponent.h"
#include "../components/ModelComponent.h"
#include "../components/TransformComponent.h"
#include "../../renderer/Model.h"
#include "../../math/mathUtils.h"
#include <algorithm>

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
        if (modelComp.ModelPtr) {
            
            // Auto-populate from model if empty
            if (animator.SkeletalAnimations.empty()) {
                animator.SkeletalAnimations = modelComp.ModelPtr->GetAnimations();
            }

            struct PRS {
                Math::Vec3f pos;
                Math::Quatf rot;
                Math::Vec3f scale;
            };

            auto sampleClip = [&](const AnimatorComponent::SkeletalAnimation& sa, float time, std::map<std::string, PRS>& outResults) {
                float ticksPerSecond = (float)sa.ticksPerSecond;
                if(ticksPerSecond == 0.0f) ticksPerSecond = 24.0f; 
                float timeInTicks = time * ticksPerSecond;
                float animationTime = fmod(timeInTicks, sa.duration);

                for (const auto& [nodeName, track] : sa.tracks) {
                    PRS prs = { {0,0,0}, Math::Quatf::Identity(), {1,1,1} };
                    
                    // Pos
                    if(track.positions.empty()) {}
                    else if(track.positions.size() == 1) prs.pos = track.positions[0].position;
                    else {
                        for(size_t i=0; i<track.positions.size()-1; i++) {
                            if(animationTime < track.positions[i+1].timeStamp) {
                                float t = (animationTime - track.positions[i].timeStamp) / (track.positions[i+1].timeStamp - track.positions[i].timeStamp);
                                prs.pos = Math::Lerp(track.positions[i].position, track.positions[i+1].position, t);
                                break;
                            }
                        }
                    }

                    // Rot
                    if(track.rotations.empty()) {}
                    else if(track.rotations.size() == 1) prs.rot = track.rotations[0].orientation;
                    else {
                        for(size_t i=0; i<track.rotations.size()-1; i++) {
                            if(animationTime < track.rotations[i+1].timeStamp) {
                                float t = (animationTime - track.rotations[i].timeStamp) / (track.rotations[i+1].timeStamp - track.rotations[i].timeStamp);
                                prs.rot = Math::Quatf::Slerp(track.rotations[i].orientation, track.rotations[i+1].orientation, t);
                                break;
                            }
                        }
                    }

                    // Scale
                    if(track.scales.empty()) {}
                    else if(track.scales.size() == 1) prs.scale = track.scales[0].scale;
                    else {
                        for(size_t i=0; i<track.scales.size()-1; i++) {
                            if(animationTime < track.scales[i+1].timeStamp) {
                                float t = (animationTime - track.scales[i].timeStamp) / (track.scales[i+1].timeStamp - track.scales[i].timeStamp);
                                prs.scale = Math::Lerp(track.scales[i].scale, track.scales[i+1].scale, t);
                                break;
                            }
                        }
                    }
                    outResults[nodeName] = prs;
                }
            };

            std::map<std::string, PRS> finalPRS;
            animator.StateTime += dt;

            // 1. Handle BlendTree1D
            if (animator.BlendTrees1D.count(animator.CurrentState)) {
                auto &bt = animator.BlendTrees1D[animator.CurrentState];
                float paramVal = animator.FloatParams.count(bt.Parameter) ? animator.FloatParams[bt.Parameter] : 0.0f;
                
                if (bt.Points.size() >= 2) {
                    // Find range
                    int idxA = -1, idxB = -1;
                    for (int i=0; i < (int)bt.Points.size()-1; i++) {
                        if (paramVal >= bt.Points[i].Value && paramVal <= bt.Points[i+1].Value) {
                            idxA = i; idxB = i+1; break;
                        }
                    }
                    if (idxA == -1) { // Clamp
                        if (paramVal < bt.Points[0].Value) { idxA = 0; idxB = 0; }
                        else { idxA = (int)bt.Points.size()-1; idxB = idxA; }
                    }

                    float weight = 0.0f;
                    if (idxA != idxB) {
                        weight = (paramVal - bt.Points[idxA].Value) / (bt.Points[idxB].Value - bt.Points[idxA].Value);
                    }

                    std::map<std::string, PRS> prsA, prsB;
                    if (animator.SkeletalAnimations.count(bt.Points[idxA].Clip)) 
                        sampleClip(animator.SkeletalAnimations.at(bt.Points[idxA].Clip), animator.StateTime, prsA);
                    if (idxA != idxB && animator.SkeletalAnimations.count(bt.Points[idxB].Clip))
                        sampleClip(animator.SkeletalAnimations.at(bt.Points[idxB].Clip), animator.StateTime, prsB);

                    // Blend
                    for (auto const& [name, pA] : prsA) {
                        if (idxA == idxB || prsB.count(name) == 0) {
                            finalPRS[name] = pA;
                        } else {
                            auto &pB = prsB[name];
                            finalPRS[name] = {
                                Math::Lerp(pA.pos, pB.pos, weight),
                                Math::Quatf::Slerp(pA.rot, pB.rot, weight),
                                Math::Lerp(pA.scale, pB.scale, weight)
                            };
                        }
                    }
                }
            } 
            // 2. Handle BlendTree2D
            else if (animator.BlendTrees2D.count(animator.CurrentState)) {
                auto &bt = animator.BlendTrees2D[animator.CurrentState];
                float x = animator.FloatParams.count(bt.ParameterX) ? animator.FloatParams[bt.ParameterX] : 0.0f;
                float y = animator.FloatParams.count(bt.ParameterY) ? animator.FloatParams[bt.ParameterY] : 0.0f;
                Math::Vec2f paramPos = {x, y};

                if (!bt.Points.empty()) {
                    std::vector<float> weights(bt.Points.size(), 0.0f);
                    float totalWeight = 0.0f;
                    
                    for (size_t i=0; i<bt.Points.size(); i++) {
                        float dist = Math::Distance(paramPos, bt.Points[i].Position);
                        if (dist < 0.001f) { // Exact match
                            std::fill(weights.begin(), weights.end(), 0.0f);
                            weights[i] = 1.0f;
                            totalWeight = 1.0f;
                            break;
                        }
                        weights[i] = 1.0f / (dist * dist);
                        totalWeight += weights[i];
                    }

                    if (totalWeight > 0.0f) {
                        bool first = true;
                        for (size_t i=0; i<bt.Points.size(); i++) {
                            float w = weights[i] / totalWeight;
                            if (w < 0.001f) continue;

                            std::map<std::string, PRS> clipPRS;
                            if (animator.SkeletalAnimations.count(bt.Points[i].Clip)) {
                                sampleClip(animator.SkeletalAnimations.at(bt.Points[i].Clip), animator.StateTime, clipPRS);
                                
                                for (auto const& [name, prs] : clipPRS) {
                                    if (first) {
                                        finalPRS[name] = { prs.pos * w, prs.rot * w, prs.scale * w };
                                    } else {
                                        finalPRS[name].pos += prs.pos * w;
                                        // Rotation blending is tricky for multi-clip. 
                                        // For now, simpler slerp or accumulation
                                        finalPRS[name].rot = Math::Quatf::Slerp(finalPRS[name].rot, prs.rot, w); 
                                        finalPRS[name].scale += prs.scale * w;
                                    }
                                }
                                first = false;
                            }
                        }
                    }
                }
            }
            // 3. Handle Single Clip
            else if (animator.SkeletalAnimations.count(animator.CurrentState)) {
                sampleClip(animator.SkeletalAnimations.at(animator.CurrentState), animator.StateTime, finalPRS);
            }

            // 3. Compute Hierarchy and Final Matrices
            if (!finalPRS.empty()) {
                auto &saBase = animator.SkeletalAnimations.begin()->second; 
                if (animator.SkeletalAnimations.count(animator.CurrentState)) 
                    saBase = animator.SkeletalAnimations.at(animator.CurrentState);

                // --- Root Motion Extraction ---
                if (animator.UseRootMotion && world.HasComponent<TransformComponent>(entity)) {
                    auto &transform = world.GetComponent<TransformComponent>(entity);
                    std::string rootNodeName = saBase.rootNode.name; 
                    // Note: In some models, the root motion node is a child of the root node (e.g. "RootMotion")
                    // For this implementation, we use the root node itself.
                    
                    if (finalPRS.count(rootNodeName)) {
                        auto &prs = finalPRS[rootNodeName];
                        
                        // Calculate delta from previous frame
                        Math::Vec3f deltaPos = prs.pos - animator.PrevRootPos;
                        
                        // Apply to entity transform (in local space for now, or world space if appropriate)
                        // This moves the entity physically
                        transform.position += deltaPos; 
                        
                        // Store current as previous for next frame
                        animator.PrevRootPos = prs.pos;
                        animator.PrevRootRot = prs.rot;

                        // ZERO OUT the root bone's position in the mesh to prevent double-movement
                        // The mesh stays at the entity's origin.
                        finalPRS[rootNodeName].pos = {0,0,0};
                    }
                }

                auto calculateBoneTransform = [&](auto& self, const AnimatorComponent::SkeletalAnimation::Node& node, const Math::Mat4& parentTransform) -> void {
                    Math::Mat4 nodeTransform = node.transformation;
                    if (finalPRS.count(node.name)) {
                        auto &prs = finalPRS[node.name];
                        nodeTransform = Math::Mat4::TRS(prs.pos, prs.rot.ToMat4x4(), prs.scale);
                    }

                    Math::Mat4 globalTransform = parentTransform * nodeTransform;

                    if (saBase.boneInfoMap.count(node.name)) {
                        int index = saBase.boneInfoMap.at(node.name).id;
                        Math::Mat4 offset = saBase.boneInfoMap.at(node.name).offset;
                        if(index < (int)animator.FinalBoneMatrices.size()) {
                            animator.FinalBoneMatrices[index] = globalTransform * offset;
                        }
                    }

                    for (const auto& child : node.children)
                        self(self, child, globalTransform);
                };

                calculateBoneTransform(calculateBoneTransform, saBase.rootNode, Math::Mat4::Identity());
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
