#pragma once

#include "../../math/VecTypes.h"
#include <functional>

namespace ge {
namespace ecs {

    enum class ButtonState
    {
        Normal,
        Hovered,
        Pressed,
        Disabled
    };

    /**
     * @brief Component for UI button interaction.
     */
    struct UIButtonComponent
    {
        ButtonState State = ButtonState::Normal;
        
        Math::Vec4f NormalColor = { 0.6f, 0.6f, 0.6f, 1.0f };
        Math::Vec4f HoverColor = { 0.8f, 0.8f, 0.8f, 1.0f };
        Math::Vec4f PressedColor = { 0.4f, 0.4f, 0.4f, 1.0f };
        Math::Vec4f DisabledColor = { 0.2f, 0.2f, 0.2f, 1.0f };

        // Interaction state
        bool IsMouseOver = false;
        bool IsPressed = false;
    };

} // namespace ecs
} // namespace ge
