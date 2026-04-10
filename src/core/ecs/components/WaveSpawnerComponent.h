#pragma once

#include "../../gameplay/GridPathfinder.h"
#include "../../math/VecTypes.h"

namespace ge {
namespace ecs {

struct WaveSpawnerComponent {
    gameplay::GridCoord SpawnCell = {0, 0};
    gameplay::GridCoord GoalCell = {0, 0};
    float SpawnInterval = 1.0f;
    float SpawnTimer = 0.0f;
    float WaveInterval = 6.0f;
    float WaveTimer = 0.0f;
    float EnemySpeed = 2.0f;
    int EnemyHealth = 3;
    int EnemiesPerWave = 3;
    int RemainingInWave = 0;
    bool LoopWaves = true;
    Math::Vec4f EnemyColor = {0.85f, 0.20f, 0.25f, 1.0f};
};

} // namespace ecs
} // namespace ge
