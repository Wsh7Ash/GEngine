#include "Window.h"
#include "../debug/log.h"
#include "../debug/assert.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace ge {
namespace platform {

static bool s_GLFWInitialized = false;

static void GLFWErrorCallback(int error, const char* description)
{
    GE_LOG_ERROR("GLFW Error (%d): %s", error, description);
}

Window::Window(const WindowProps& props)
{
    Init(props);
}

Window::~Window()
{
    Shutdown();
}

void Window::Init(const WindowProps& props)
{
    data_.Title = props.Title;
    data_.Width = props.Width;
    data_.Height = props.Height;

    GE_LOG_INFO("Creating window %s (%u, %u)", props.Title.c_str(), props.Width, props.Height);

    if (!s_GLFWInitialized)
    {
        int success = glfwInit();
        GE_ASSERT(success, "Could not initialize GLFW!");
        glfwSetErrorCallback(GLFWErrorCallback);
        s_GLFWInitialized = true;
    }

    // OpenGL 4.5 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window_ = glfwCreateWindow((int)props.Width, (int)props.Height, data_.Title.c_str(), nullptr, nullptr);
    GE_ASSERT(window_, "Could not create window!");

    glfwMakeContextCurrent(window_);
    
    // Initialize GLAD
    int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    GE_ASSERT(status, "Failed to initialize Glad!");

    GE_LOG_INFO("OpenGL Info:");
    GE_LOG_INFO("  Vendor:   %s", (const char*)glGetString(GL_VENDOR));
    GE_LOG_INFO("  Renderer: %s", (const char*)glGetString(GL_RENDERER));
    GE_LOG_INFO("  Version:  %s", (const char*)glGetString(GL_VERSION));

    glfwSetWindowUserPointer(window_, &data_);
    SetVSync(true);

    // Set GLFW callbacks
    glfwSetWindowSizeCallback(window_, [](GLFWwindow* window, int width, int height)
    {
        WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
        data.Width = width;
        data.Height = height;
        
        // In the future, we'll dispatch a WindowResizeEvent here
        glViewport(0, 0, width, height);
    });

    glfwSetWindowCloseCallback(window_, [](GLFWwindow* window)
    {
        WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
        // In the future, we'll dispatch a WindowCloseEvent here
    });
}

void Window::Shutdown()
{
    glfwDestroyWindow(window_);
}

void Window::OnUpdate()
{
    glfwPollEvents();
    glfwSwapBuffers(window_);
}

void Window::SetVSync(bool enabled)
{
    if (enabled)
        glfwSwapInterval(1);
    else
        glfwSwapInterval(0);

    data_.VSync = enabled;
}

bool Window::ShouldClose() const
{
    return glfwWindowShouldClose(window_);
}

} // namespace platform
} // namespace ge
