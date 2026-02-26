#include "Framebuffer.h"
#include "RendererAPI.h"
#include "opengl/OpenGLFramebuffer.h"

namespace ge {
namespace renderer {

    std::shared_ptr<Framebuffer> Framebuffer::Create(const FramebufferSpecification& spec)
    {
        switch (RendererAPI::GetAPI())
        {
            case RenderAPI::None:    return nullptr;
            case RenderAPI::OpenGL:  return std::make_shared<OpenGLFramebuffer>(spec);
            case RenderAPI::DX11:    return nullptr; // Not implemented
        }

        return nullptr;
    }

} // namespace renderer
} // namespace ge
