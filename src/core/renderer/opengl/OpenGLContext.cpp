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
        GE_LOG_INFO("Initializing OpenGL context...");
        glfwMakeContextCurrent(windowHandle_);
        int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
        if (!status)
        {
            GE_LOG_CRITICAL("CRITICAL: Failed to initialize Glad!");
            std::abort();
        }

        GE_LOG_INFO("OpenGL Info:");
        const char* vendor = (const char*)glGetString(GL_VENDOR);
        const char* renderer = (const char*)glGetString(GL_RENDERER);
        const char* version = (const char*)glGetString(GL_VERSION);

        if (vendor) GE_LOG_INFO("  Vendor:   %s", vendor);
        else GE_LOG_ERROR("  Vendor:   NULL (Context failed?)");

        if (renderer) GE_LOG_INFO("  Renderer: %s", renderer);
        if (version) GE_LOG_INFO("  Version:  %s", version);
    }

    void OpenGLContext::SwapBuffers()
    {
        glfwSwapBuffers(windowHandle_);
    }

} // namespace renderer
} // namespace ge
