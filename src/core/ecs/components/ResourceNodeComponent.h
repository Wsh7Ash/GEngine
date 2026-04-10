#pragma once

#include <string>

namespace ge {
namespace ecs {

struct ResourceNodeComponent {
    std::string ItemId = "resource.wood";
    int Amount = 5;
    int MaxAmount = 5;
    int YieldPerInteract = 1;
    float RespawnDelay = 0.0f;
    float RespawnTimer = 0.0f;
    bool Depleted = false;
};

} // namespace ecs
} // namespace ge
