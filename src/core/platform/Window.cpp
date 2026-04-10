#include "Window.h"
#include "../debug/log.h"
#include "../debug/assert.h"
#include "../renderer/RendererAPI.h"
#include "../renderer/opengl/OpenGLContext.h"

#if defined(GE_ENABLE_DX11_BACKEND) && GE_ENABLE_DX11_BACKEND
#include "../renderer/dx11/DX11Context.h"
#endif

#if defined(GE_ENABLE_WEBGL2_BACKEND) && GE_ENABLE_WEBGL2_BACKEND
#include "../renderer/webgl2/WebGL2Context.h"
#endif

#if defined(GE_ENABLE_VULKAN_BACKEND) && GE_ENABLE_VULKAN_BACKEND
#include "../renderer/vulkan/VulkanContext.h"
#endif

#include <GLFW/glfw3.h>

#ifdef GE_PLATFORM_WINDOWS
    #define GLFW_EXPOSE_NATIVE_WIN32
    #include <GLFW/glfw3native.h>
    #include <Windows.h>
#elif defined(GE_PLATFORM_LINUX)
    #define GLFW_EXPOSE_NATIVE_X11
    #define GLFW_EXPOSE_NATIVE_WAYLAND
    #include <GLFW/glfw3native.h>
#elif defined(GE_PLATFORM_MACOS)
    #define GLFW_EXPOSE_NATIVE_COCOA
    #include <GLFW/glfw3native.h>
#endif

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

    // Set Window Hints based on API
    if (renderer::RendererAPI::GetAPI() == renderer::RenderAPI::OpenGL)
    {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    }
    else if (renderer::RendererAPI::GetAPI() == renderer::RenderAPI::DX11)
    {
#if defined(GE_ENABLE_DX11_BACKEND) && GE_ENABLE_DX11_BACKEND
        // No client API for DX11 context
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#else
        GE_LOG_ERROR("DX11 backend requested but not compiled into this build.");
#endif
    }
    else if (renderer::RendererAPI::GetAPI() == renderer::RenderAPI::Vulkan)
    {
#if defined(GE_ENABLE_VULKAN_BACKEND) && GE_ENABLE_VULKAN_BACKEND
        // Vulkan uses GLFW_NO_API - no OpenGL context needed
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#else
        GE_LOG_ERROR("Vulkan backend requested but not compiled into this build.");
#endif
    }
    else if (renderer::RendererAPI::GetAPI() == renderer::RenderAPI::WebGL2)
    {
#if defined(GE_ENABLE_WEBGL2_BACKEND) && GE_ENABLE_WEBGL2_BACKEND
        // WebGL2 uses OpenGL ES 3.0 context via GLFW
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
        glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
#else
        GE_LOG_ERROR("WebGL2 backend requested but not compiled into this build.");
#endif
    }

    window_ = glfwCreateWindow((int)props.Width, (int)props.Height, data_.Title.c_str(), nullptr, nullptr);
    GE_ASSERT(window_, "Could not create window!");

    // Create Graphics Context
    if (renderer::RendererAPI::GetAPI() == renderer::RenderAPI::OpenGL)
    {
        context_ = std::make_unique<renderer::OpenGLContext>(window_);
    }
    else if (renderer::RendererAPI::GetAPI() == renderer::RenderAPI::DX11)
    {
#if defined(GE_ENABLE_DX11_BACKEND) && GE_ENABLE_DX11_BACKEND
        context_ = std::make_unique<renderer::DX11Context>(window_);
#else
        GE_ASSERT(false, "DX11 backend requested but not compiled into this build");
#endif
    }
    else if (renderer::RendererAPI::GetAPI() == renderer::RenderAPI::Vulkan)
    {
#if defined(GE_ENABLE_VULKAN_BACKEND) && GE_ENABLE_VULKAN_BACKEND
        context_ = std::make_unique<renderer::VulkanContext>(window_);
#else
        GE_ASSERT(false, "Vulkan backend requested but not compiled into this build");
#endif
    }
    else if (renderer::RendererAPI::GetAPI() == renderer::RenderAPI::WebGL2)
    {
#if defined(GE_ENABLE_WEBGL2_BACKEND) && GE_ENABLE_WEBGL2_BACKEND
        context_ = std::make_unique<renderer::WebGL2Context>(window_);
#else
        GE_ASSERT(false, "WebGL2 backend requested but not compiled into this build");
#endif
    }

    context_->Init();

    glfwSetWindowUserPointer(window_, &data_);
    SetVSync(true);

    // Set GLFW callbacks
    glfwSetWindowSizeCallback(window_, [](GLFWwindow* window, int width, int height)
    {
        WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
        data.Width = width;
        data.Height = height;
        
        // In the future, we'll dispatch a WindowResizeEvent here
        // For GL we might need glViewport, but that should probably stay in RenderAPI
    });

    glfwSetWindowCloseCallback(window_, [](GLFWwindow* window)
    {
        (void)window;
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
    context_->SwapBuffers();
}

void Window::SetVSync(bool enabled)
{
    if (renderer::RendererAPI::GetAPI() == renderer::RenderAPI::OpenGL)
    {
        if (enabled)
            glfwSwapInterval(1);
        else
            glfwSwapInterval(0);
    }

    data_.VSync = enabled;
}

bool Window::ShouldClose() const
{
    return glfwWindowShouldClose(window_);
}

} // namespace platform
} // namespace ge
