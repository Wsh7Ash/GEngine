#pragma once

#include <string>

namespace ge {
namespace ecs {

struct InteractionComponent {
    float Range = 1.25f;
    bool Enabled = true;
    std::string Prompt = "Interact";
};

} // namespace ecs
} // namespace ge
