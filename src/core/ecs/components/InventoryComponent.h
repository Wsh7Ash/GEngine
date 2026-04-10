#pragma once

#include <string>
#include <vector>

namespace ge {
namespace ecs {

struct InventoryStack {
    std::string ItemId;
    int Quantity = 0;
};

struct InventoryComponent {
    int Capacity = 16;
    std::vector<InventoryStack> Items;
};

} // namespace ecs
} // namespace ge
