#pragma once

#include "../../math/VecTypes.h"
#include <vector>

namespace ge {
namespace ecs {

struct AudioObstructionRay {
    Math::Vec3f Direction;
    float Distance;
    float ObstructionAmount;
};

struct AudioListenerComponent {
    bool IsActive = true;
    Math::Vec3f Position = Math::Vec3f::Zero();
    Math::Vec3f Forward = Math::Vec3f::Forward();
    Math::Vec3f Up = Math::Vec3f::Up();

    bool EnableObstruction = true;
    float MaxObstructionDistance = 100.0f;
    int ObstructionRays = 8;
    bool EnableLowpassOnObstruction = true;
    float LowpassMinFrequency = 800.0f;
    float LowpassMaxFrequency = 22050.0f;
    float ObstructionTransitionTime = 0.1f;

    std::vector<AudioObstructionRay> LastObstructionRays;
};

} // namespace ecs
} // namespace ge
