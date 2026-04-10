#pragma once

#include "../../gameplay/GridPathfinder.h"
#include <cstdint>
#include <vector>

namespace ge {
namespace ecs {

struct GridMapComponent {
    gameplay::GridPathData Grid;
};

struct PathAgentComponent {
    gameplay::GridCoord GoalCell = {0, 0};
    float MoveSpeed = 2.5f;
    std::vector<gameplay::GridCoord> Path;
    uint32_t NextWaypointIndex = 0;
    bool RepathRequired = true;
    bool DestroyAtGoal = true;
};

} // namespace ecs
} // namespace ge
