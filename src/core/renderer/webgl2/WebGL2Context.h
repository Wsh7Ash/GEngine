#pragma once

#include "../GraphicsContext.h"
#include <GLFW/glfw3.h>

namespace ge {
namespace renderer {

class WebGL2Context : public GraphicsContext {
public:
    WebGL2Context(GLFWwindow* window);
    virtual ~WebGL2Context() override;

    virtual void Init() override;
    virtual void SwapBuffers() override;

    GLFWwindow* GetWindow() const { return window_; }

private:
    GLFWwindow* window_;
    bool initialized_ = false;
};

} // namespace renderer
} // namespace ge
