#pragma once

#include "../System.h"
#include "../World.h"
#include "../components/AnimatorComponent.h"

namespace ge {
namespace ecs {

/**
 * @brief System that handles 2D animation state transitions and frame updates.
 */
class AnimationSystem : public System {
public:
  void Update(World &world, float dt);

private:
  bool EvaluateTransition(const AnimatorComponent &animator,
                          const AnimationTransition &transition);
};

} // namespace ecs
} // namespace ge
