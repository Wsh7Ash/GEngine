#pragma once

#include "../../math/VecTypes.h"
#include <map>
#include <string>
#include <vector>
#include <memory>
#include "../../math/Mat4x4.h"
#include "../../math/quaternion.h"

namespace ge {
namespace ecs {

/**
 * @brief Represents a single frame in a 2D animation.
 */
struct Animation2DFrame {
  int Index;
  float Duration; // Seconds
};

/**
 * @brief Represents a full 2D animation clip.
 */
struct Animation2D {
  std::string Name;
  std::vector<Animation2DFrame> Frames;
  bool Loop = true;
};

/**
 * @brief Represents a transition between states.
 */
struct AnimationTransition {
  std::string TargetState;
  std::string ConditionParameter;

  enum class ConditionOperator {
    Greater,
    Less,
    Equal,
    NotEqual,
    IsTrue,
    IsFalse
  };

  ConditionOperator Op;
  float Threshold = 0.0f;
};

/**
 * @brief Animator component for state-machine based 2D animation.
 */
struct AnimatorComponent {
  std::map<std::string, Animation2D> Animations;
  std::map<std::string, std::vector<AnimationTransition>> Transitions;

  std::string CurrentState;
  float StateTime = 0.0f;
  int CurrentFrameIndex = 0;

  // Parameters
  std::map<std::string, float> FloatParams;
  std::map<std::string, bool> BoolParams;

  // Helper to set parameter
  void SetFloat(const std::string &name, float value) { FloatParams[name] = value; }
  void SetBool(const std::string &name, bool value) { BoolParams[name] = value; }

  // ── 3D Skeletal Animation ─────────────────────────────────────
  
  struct BoneInfo {
      int id;
      Math::Mat4 offset; // Offset matrix to transform from model space to bone space
  };

  struct KeyPosition {
      Math::Vec3f position;
      float timeStamp;
  };

  struct KeyRotation {
      Math::Quatf orientation;
      float timeStamp;
  };

  struct KeyScale {
      Math::Vec3f scale;
      float timeStamp;
  };

  struct BoneTrack {
      std::vector<KeyPosition> positions;
      std::vector<KeyRotation> rotations;
      std::vector<KeyScale> scales;
      Math::Mat4 localTransform;
      std::string name;
      int id;
  };

  struct SkeletalAnimation {
      float duration;
      int ticksPerSecond;
      std::map<std::string, BoneTrack> tracks;
      
      struct Node {
          std::string name;
          Math::Mat4 transformation;
          std::vector<Node> children;
      };
      Node rootNode;
      std::map<std::string, BoneInfo> boneInfoMap;
  };

  struct BlendTree1D {
      std::string Parameter;
      struct Point {
          float Value;
          std::string Clip;
      };
      std::vector<Point> Points; // Sorted by Value
  };

  struct BlendTree2D {
      std::string ParameterX;
      std::string ParameterY;
      struct Point {
          Math::Vec2f Position;
          std::string Clip;
      };
      std::vector<Point> Points;
  };

  std::map<std::string, SkeletalAnimation> SkeletalAnimations;
  std::map<std::string, BlendTree1D> BlendTrees1D;
  std::map<std::string, BlendTree2D> BlendTrees2D;

  std::vector<Math::Mat4> FinalBoneMatrices;
  bool Is3D = false;

  // Root Motion state
  bool UseRootMotion = false;
  int RootBoneIndex = 0;
  Math::Vec3f RootDelta = {0,0,0}; // Extracted displacement for this frame
  Math::Vec3f PrevRootPos = {0,0,0};
  Math::Quatf PrevRootRot = Math::Quatf::Identity();

  AnimatorComponent() {
      FinalBoneMatrices.reserve(100);
      for (int i = 0; i < 100; i++)
          FinalBoneMatrices.push_back(Math::Mat4::Identity());
  }
};

} // namespace ecs
} // namespace ge
