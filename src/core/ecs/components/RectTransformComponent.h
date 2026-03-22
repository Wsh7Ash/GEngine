#pragma once

#include "../../math/VecTypes.h"

namespace ge {
namespace ecs {

    /**
     * @brief Anchor presets for UI elements.
     */
    enum class AnchorPreset
    {
        TopLeft, TopCenter, TopRight,
        CenterLeft, Center, CenterRight,
        BottomLeft, BottomCenter, BottomRight,
        Stretch
    };

    /**
     * @brief Component for UI layout and positioning in screen space.
     */
    struct RectTransformComponent
    {
        Math::Vec2f Position = { 0.0f, 0.0f };
        Math::Vec2f Size = { 100.0f, 100.0f };
        Math::Vec2f AnchorMin = { 0.5f, 0.5f }; // Normalized (0-1)
        Math::Vec2f AnchorMax = { 0.5f, 0.5f }; // Normalized (0-1)
        Math::Vec2f Pivot = { 0.5f, 0.5f };     // Normalized (0-1)
        float Rotation = 0.0f;

        // Calculated screen-space bounds
        Math::Vec2f ScreenPosition = { 0.0f, 0.0f };
        Math::Vec2f ScreenSize = { 100.0f, 100.0f };
    };

} // namespace ecs
} // namespace ge
