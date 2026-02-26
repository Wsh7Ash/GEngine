#include "Window.h"
#include "../debug/log.h"
#include "../debug/assert.h"
#include "../renderer/RendererAPI.h"
#include "../renderer/opengl/OpenGLContext.h"
#include "../renderer/dx11/DX11Context.h"

#include <GLFW/glfw3.h>

#ifdef GE_PLATFORM_WINDOWS
    #define GLFW_EXPOSE_NATIVE_WIN32
    #include <GLFW/glfw3native.h>
    #include <Windows.h>
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
        // No client API for DX11 context
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
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
        context_ = std::make_unique<renderer::DX11Context>(window_);
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

void Window::InitNativeMenuBar()
{
#ifdef GE_PLATFORM_WINDOWS
    HWND hwnd = (HWND)GetHWND();
    HMENU hMenuBar = CreateMenu();
    
    // File Menu
    HMENU hFileMenu = CreatePopupMenu();
    AppendMenu(hFileMenu, MF_STRING, 1001, "New Scene");
    AppendMenu(hFileMenu, MF_STRING, 1002, "Open Scene...");
    AppendMenu(hFileMenu, MF_STRING, 1003, "Save Scene");
    AppendMenu(hFileMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hFileMenu, MF_STRING, 1004, "Exit");
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hFileMenu, "File");

    // Edit Menu
    HMENU hEditMenu = CreatePopupMenu();
    AppendMenu(hEditMenu, MF_STRING, 2001, "Undo");
    AppendMenu(hEditMenu, MF_STRING, 2002, "Redo");
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hEditMenu, "Edit");

    SetMenu(hwnd, hMenuBar);
#endif
}

void* Window::GetHWND() const
{
#ifdef GE_PLATFORM_WINDOWS
    return glfwGetWin32Window(window_);
#else
    return nullptr;
#endif
}

} // namespace platform
} // namespace ge
