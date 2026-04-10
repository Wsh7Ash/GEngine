#pragma once

namespace ge {
namespace ecs {

struct HealthComponent {
    int Current = 1;
    int Max = 1;
    bool DestroyOnZero = true;
};

} // namespace ecs
} // namespace ge
