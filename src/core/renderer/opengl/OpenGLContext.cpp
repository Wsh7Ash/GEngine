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

#ifndef NDEBUG
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity,
            GLsizei length, const GLchar* message, const void* userParam) {
            (void)source; (void)id; (void)length; (void)userParam;
            const char* typeStr = "UNKNOWN";
            switch (type) {
                case GL_DEBUG_TYPE_ERROR:               typeStr = "ERROR"; break;
                case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:  typeStr = "DEPRECATED"; break;
                case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:   typeStr = "UNDEFINED"; break;
                case GL_DEBUG_TYPE_PORTABILITY:         typeStr = "PORTABILITY"; break;
                case GL_DEBUG_TYPE_PERFORMANCE:         typeStr = "PERFORMANCE"; break;
                case GL_DEBUG_TYPE_MARKER:               typeStr = "MARKER"; break;
                case GL_DEBUG_TYPE_PUSH_GROUP:           typeStr = "PUSH_GROUP"; break;
                case GL_DEBUG_TYPE_POP_GROUP:            typeStr = "POP_GROUP"; break;
                case GL_DEBUG_TYPE_OTHER:               typeStr = "OTHER"; break;
            }
            const char* severityStr = "DEBUG";
            switch (severity) {
                case GL_DEBUG_SEVERITY_HIGH:         severityStr = "HIGH"; break;
                case GL_DEBUG_SEVERITY_MEDIUM:       severityStr = "MEDIUM"; break;
                case GL_DEBUG_SEVERITY_LOW:          severityStr = "LOW"; break;
                case GL_DEBUG_SEVERITY_NOTIFICATION: severityStr = "NOTIFY"; break;
            }
            if (severity == GL_DEBUG_SEVERITY_HIGH || severity == GL_DEBUG_SEVERITY_MEDIUM) {
                GE_LOG_ERROR("[GL %s/%s] %s", typeStr, severityStr, message);
            } else if (severity == GL_DEBUG_SEVERITY_LOW) {
                GE_LOG_WARN("[GL %s/%s] %s", typeStr, severityStr, message);
            } else {
                GE_LOG_DEBUG("[GL %s/%s] %s", typeStr, severityStr, message);
            }
        }, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
        GE_LOG_INFO("OpenGL debug output enabled (KHR_debug)");
#endif
    }

    void OpenGLContext::SwapBuffers()
    {
        glfwSwapBuffers(windowHandle_);
    }

} // namespace renderer
} // namespace ge
