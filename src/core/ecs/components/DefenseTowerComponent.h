#pragma once

namespace ge {
namespace ecs {

struct DefenseTowerComponent {
    float Range = 3.5f;
    float FireInterval = 0.5f;
    float Cooldown = 0.0f;
    int Damage = 1;
};

} // namespace ecs
} // namespace ge
