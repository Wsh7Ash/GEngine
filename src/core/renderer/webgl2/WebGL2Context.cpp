#include "WebGL2Context.h"
#include "../../debug/log.h"
#include "../../debug/assert.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/html5.h>
#endif

namespace ge {
namespace renderer {

static bool s_ContextLost = false;
static std::function<void()> s_OnContextLost;
static std::function<void()> s_OnContextRestored;

#ifdef __EMSCRIPTEN__
static EM_BOOL ContextLostCallback(int eventType, const void* reserved, void* userData) {
    (void)eventType;
    (void)reserved;
    (void)userData;
    
    s_ContextLost = true;
    GE_LOG_WARN("WebGL2 Context lost");
    
    if (s_OnContextLost) {
        s_OnContextLost();
    }
    return EM_TRUE;
}

static EM_BOOL ContextRestoredCallback(int eventType, const void* reserved, void* userData) {
    (void)eventType;
    (void)reserved;
    (void)userData;
    
    s_ContextLost = false;
    GE_LOG_INFO("WebGL2 Context restored");
    
    if (s_OnContextRestored) {
        s_OnContextRestored();
    }
    return EM_TRUE;
}
#endif

WebGL2Context::WebGL2Context(GLFWwindow* window)
    : window_(window)
{
}

WebGL2Context::~WebGL2Context() {
#ifdef __EMSCRIPTEN__
    emscripten_set_webglcontextlost_callback("#canvas", nullptr, EM_TRUE, nullptr);
    emscripten_set_webglcontextrestored_callback("#canvas", nullptr, EM_TRUE, nullptr);
#endif
}

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

    emscripten_set_webglcontextlost_callback("#canvas", nullptr, EM_TRUE, ContextLostCallback);
    emscripten_set_webglcontextrestored_callback("#canvas", nullptr, EM_TRUE, ContextRestoredCallback);

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

void WebGL2Context::SetContextLostCallback(std::function<void()> callback) {
    onContextLost_ = std::move(callback);
#ifdef __EMSCRIPTEN__
    s_OnContextLost = onContextLost_;
#endif
}

void WebGL2Context::SetContextRestoredCallback(std::function<void()> callback) {
    onContextRestored_ = std::move(callback);
#ifdef __EMSCRIPTEN__
    s_OnContextRestored = onContextRestored_;
#endif
}

void WebGL2Context::RecreateResources() {
    if (!contextLost_) return;
    
    GE_LOG_INFO("Recreating WebGL resources after context restore");
    
#ifdef __EMSCRIPTEN__
    contextLost_ = false;
    initialized_ = true;
#endif
}

} // namespace renderer
} // namespace ge
