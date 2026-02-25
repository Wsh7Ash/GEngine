#include "GLFWInput.h"
#include "Window.h"
#include <GLFW/glfw3.h>

namespace ge {
namespace platform {

// For now, we assume there's only one window and we can get it globally or from a static engine instance.
// Since we don't have a Global Engine class yet, we'll need a way to get the current window.
// I'll add a way to get the GLFWwindow* from our Window class.

// We need a way to track the active window. For now, I'll store a static reference.
static GLFWwindow* s_ActiveGLFWWindow = nullptr;

void InitializeInput(Window* window)
{
    s_ActiveGLFWWindow = static_cast<GLFWwindow*>(window->GetNativeWindow());
}

Input* Input::s_Instance = new GLFWInput();

bool GLFWInput::IsKeyPressedImpl(int keycode)
{
    if (!s_ActiveGLFWWindow) return false;
    auto state = glfwGetKey(s_ActiveGLFWWindow, keycode);
    return state == GLFW_PRESS || state == GLFW_REPEAT;
}

bool GLFWInput::IsMouseButtonPressedImpl(int button)
{
    if (!s_ActiveGLFWWindow) return false;
    auto state = glfwGetMouseButton(s_ActiveGLFWWindow, button);
    return state == GLFW_PRESS;
}

std::pair<float, float> GLFWInput::GetMousePositionImpl()
{
    if (!s_ActiveGLFWWindow) return { 0.0f, 0.0f };
    double xpos, ypos;
    glfwGetCursorPos(s_ActiveGLFWWindow, &xpos, &ypos);
    return { (float)xpos, (float)ypos };
}

float GLFWInput::GetMouseXImpl()
{
    auto [x, y] = GetMousePositionImpl();
    return x;
}

float GLFWInput::GetMouseYImpl()
{
    auto [x, y] = GetMousePositionImpl();
    return y;
}

} // namespace platform
} // namespace ge
