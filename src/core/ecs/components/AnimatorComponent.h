#pragma once

#include "../../math/VecTypes.h"
#include <map>
#include <string>
#include <vector>

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
};

} // namespace ecs
} // namespace ge
