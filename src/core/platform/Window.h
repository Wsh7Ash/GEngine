#pragma once

#include "renderer/GraphicsContext.h"
#include <string>
#include <functional>
#include <memory>

// Forward declaration of GLFWwindow
struct GLFWwindow;

namespace ge {
namespace platform {

struct WindowProps
{
    std::string Title;
    uint32_t Width;
    uint32_t Height;

    WindowProps(const std::string& title = "GEngine",
                uint32_t width = 1280,
                uint32_t height = 720)
        : Title(title), Width(width), Height(height)
    {
    }
};

/**
 * @brief Cross-platform window abstraction using GLFW.
 */
class Window
{
public:
    using EventCallbackFn = std::function<void()>; // Placeholder for event system

    Window(const WindowProps& props);
    ~Window();

    void OnUpdate();

    [[nodiscard]] uint32_t GetWidth() const { return data_.Width; }
    [[nodiscard]] uint32_t GetHeight() const { return data_.Height; }

    // Window attributes
    void SetEventCallback(const EventCallbackFn& callback) { data_.EventCallback = callback; }
    void SetVSync(bool enabled);
    [[nodiscard]] bool IsVSync() const { return data_.VSync; }

    [[nodiscard]] bool ShouldClose() const;
    
    [[nodiscard]] void* GetNativeWindow() const { return window_; }

    void InitNativeMenuBar();

private:
    void Init(const WindowProps& props);
    void Shutdown();
    void* GetHWND() const;

private:
    GLFWwindow* window_;
    std::unique_ptr<renderer::GraphicsContext> context_;

    struct WindowData
    {
        std::string Title;
        uint32_t Width, Height;
        bool VSync;

        EventCallbackFn EventCallback;
    };

    WindowData data_;
};

} // namespace platform
} // namespace ge
