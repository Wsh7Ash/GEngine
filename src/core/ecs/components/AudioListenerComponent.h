#pragma once

namespace ge {
namespace ecs {

struct AudioListenerComponent {
    bool IsActive = true;  // Only one listener should be active at a time
};

} // namespace ecs
} // namespace ge
