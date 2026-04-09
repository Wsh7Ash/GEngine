#pragma once

// ================================================================
//  WebGL2Context.h
//  WebGL2 rendering context for web browsers.
// ================================================================

/**
 * @file WebGL2Context.h
 * @brief WebGL2 context with Emscripten integration and context loss handling.
 * 
 * Supports:
 * - Emscripten WebGL context creation
 * - Context loss/restore events
 * - WebGL2 features (UBO, VAO, MRT, etc.)
 */

#include "../GraphicsContext.h"
#include <GLFW/glfw3.h>
#include <functional>

namespace ge {
namespace renderer {

class WebGL2Context : public GraphicsContext {
public:
    WebGL2Context(GLFWwindow* window);
    virtual ~WebGL2Context() override;

    virtual void Init() override;
    virtual void SwapBuffers() override;

    GLFWwindow* GetWindow() const { return window_; }

    bool IsContextLost() const { return contextLost_; }
    void SetContextLostCallback(std::function<void()> callback);
    void SetContextRestoredCallback(std::function<void()> callback);

    void RecreateResources();

private:
    void SetupContextCallbacks();

    GLFWwindow* window_;
    bool initialized_ = false;
    bool contextLost_ = false;
    
    std::function<void()> onContextLost_;
    std::function<void()> onContextRestored_;
};

} // namespace renderer
} // namespace ge
