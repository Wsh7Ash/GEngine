#pragma once

#include "Input.h"

namespace ge {
namespace platform {

/**
 * @brief GLFW-specific implementation of the Input class.
 */
class GLFWInput : public Input
{
protected:
    virtual bool IsKeyPressedImpl(int keycode) override;
    virtual bool IsMouseButtonPressedImpl(int button) override;
    virtual std::pair<float, float> GetMousePositionImpl() override;
    virtual float GetMouseXImpl() override;
    virtual float GetMouseYImpl() override;
};

void InitializeInput(class Window* window);

} // namespace platform
} // namespace ge
