#include "Framebuffer.h"
#include "RendererAPI.h"
#include "opengl/OpenGLFramebuffer.h"
#include "webgl2/WebGL2Framebuffer.h"

namespace ge {
namespace renderer {

    std::shared_ptr<Framebuffer> Framebuffer::Create(const FramebufferSpecification& spec)
    {
        switch (RendererAPI::GetAPI())
        {
            case RenderAPI::None:    return nullptr;
            case RenderAPI::OpenGL:  return std::make_shared<OpenGLFramebuffer>(spec);
            case RenderAPI::DX11:    return nullptr; // Not implemented
            case RenderAPI::WebGL2:  return std::make_shared<WebGL2Framebuffer>(spec);
        }

        return nullptr;
    }

} // namespace renderer
} // namespace ge
