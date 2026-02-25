#pragma once

#include "../GraphicsContext.h"
#include <glad/glad.h>

struct GLFWwindow;

namespace ge {
namespace renderer {

    class OpenGLContext : public GraphicsContext
    {
    public:
        OpenGLContext(GLFWwindow* windowHandle);

        virtual void Init() override;
        virtual void SwapBuffers() override;

    private:
        GLFWwindow* windowHandle_;
    };

} // namespace renderer
} // namespace ge
