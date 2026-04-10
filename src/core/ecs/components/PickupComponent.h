#pragma once

#include <string>

namespace ge {
namespace ecs {

struct PickupComponent {
    std::string ItemId = "resource.wood";
    int Quantity = 1;
    float AutoPickupRadius = 0.9f;
};

} // namespace ecs
} // namespace ge
