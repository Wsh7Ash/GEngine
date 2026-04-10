#pragma once

#include "../../math/VecTypes.h"

namespace ge {
namespace ecs {

struct BuildPlacementComponent {
    bool BlocksPath = true;
    bool DisallowBlockingPath = true;
    float PlacementCooldown = 0.15f;
    float PlacementTimer = 0.0f;
    int RemainingPlacements = -1;
    Math::Vec4f DefenseColor = {0.95f, 0.80f, 0.25f, 1.0f};
};

} // namespace ecs
} // namespace ge
