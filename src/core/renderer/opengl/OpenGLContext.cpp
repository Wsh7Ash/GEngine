#include "OpenGLContext.h"
#include "../../debug/assert.h"
#include "../../debug/log.h"
#include <GLFW/glfw3.h>

namespace ge {
namespace renderer {

    OpenGLContext::OpenGLContext(GLFWwindow* windowHandle)
        : windowHandle_(windowHandle)
    {
        GE_ASSERT(windowHandle, "Window handle is null!");
    }

    void OpenGLContext::Init()
    {
        glfwMakeContextCurrent(windowHandle_);
        int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
        GE_ASSERT(status, "Failed to initialize Glad!");

        GE_LOG_INFO("OpenGL Info:");
        GE_LOG_INFO("  Vendor:   %s", glGetString(GL_VENDOR));
        GE_LOG_INFO("  Renderer: %s", glGetString(GL_RENDERER));
        GE_LOG_INFO("  Version:  %s", glGetString(GL_VERSION));
    }

    void OpenGLContext::SwapBuffers()
    {
        glfwSwapBuffers(windowHandle_);
    }

} // namespace renderer
} // namespace ge
