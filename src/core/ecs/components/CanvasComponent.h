#pragma once

#include <cstdint>

namespace ge {
namespace ecs {

    enum class RenderMode
    {
        ScreenSpaceOverlay,
        ScreenSpaceCamera,
        WorldSpace
    };

    /**
     * @brief Component that marks an entity as a UI root (Canvas).
     */
    struct CanvasComponent
    {
        RenderMode Mode = RenderMode::ScreenSpaceOverlay;
        uint32_t SortingOrder = 0;
    };

} // namespace ecs
} // namespace ge
