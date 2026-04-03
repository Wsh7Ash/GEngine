#include "WebGL2Context.h"
#include "../../debug/log.h"
#include "../../debug/assert.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/html5.h>
#endif

namespace ge {
namespace renderer {

WebGL2Context::WebGL2Context(GLFWwindow* window)
    : window_(window)
{
}

WebGL2Context::~WebGL2Context() = default;

void WebGL2Context::Init()
{
    if (initialized_) return;

#ifdef __EMSCRIPTEN__
    GE_LOG_INFO("WebGL2 Context: Initializing WebGL 2.0");

    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);
    attrs.alpha = true;
    attrs.depth = true;
    attrs.stencil = true;
    attrs.antialias = true;
    attrs.premultipliedAlpha = false;
    attrs.preserveDrawingBuffer = false;
    attrs.powerPreference = "high-performance";
    attrs.failIfMajorPerformanceCaveat = false;
    attrs.enableExtensionsByDefault = true;

    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx = emscripten_webgl_create_context("#canvas", &attrs);
    if (!ctx) {
        GE_LOG_ERROR("Failed to create WebGL context");
        return;
    }

    EMSCRIPTEN_RESULT res = emscripten_webgl_make_context_current(ctx);
    if (res != EMSCRIPTEN_RESULT_SUCCESS) {
        GE_LOG_ERROR("Failed to make WebGL context current");
        return;
    }

    initialized_ = true;
    GE_LOG_INFO("WebGL2 Context initialized successfully");
#else
    GE_LOG_WARN("WebGL2 Context: Not running under Emscripten - using fallback");
    glfwMakeContextCurrent(window_);
    initialized_ = true;
#endif
}

void WebGL2Context::SwapBuffers()
{
#ifdef __EMSCRIPTEN__
    // WebGL doesn't require explicit buffer swapping - handled by browser
#else
    if (window_) {
        glfwSwapBuffers(window_);
    }
#endif
}

} // namespace renderer
} // namespace ge
