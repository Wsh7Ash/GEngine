#include "Framebuffer.h"
#include "RendererAPI.h"
#include "opengl/OpenGLFramebuffer.h"

#if defined(GE_ENABLE_WEBGL2_BACKEND) && GE_ENABLE_WEBGL2_BACKEND
#include "webgl2/WebGL2Framebuffer.h"
#endif

#if defined(GE_ENABLE_DX11_BACKEND) && GE_ENABLE_DX11_BACKEND
#include "dx11/DX11Framebuffer.h"
#endif

#if defined(GE_ENABLE_VULKAN_BACKEND) && GE_ENABLE_VULKAN_BACKEND
#include "vulkan/VulkanFramebuffer.h"
#endif

namespace ge {
namespace renderer {

    std::shared_ptr<Framebuffer> Framebuffer::Create(const FramebufferSpecification& spec)
    {
        switch (RendererAPI::GetAPI())
        {
            case RenderAPI::None:    return nullptr;
            case RenderAPI::OpenGL:  return std::make_shared<OpenGLFramebuffer>(spec);
            case RenderAPI::DX11:
#if defined(GE_ENABLE_DX11_BACKEND) && GE_ENABLE_DX11_BACKEND
                return std::make_shared<DX11Framebuffer>(spec);
#else
                return nullptr;
#endif
            case RenderAPI::Vulkan:
#if defined(GE_ENABLE_VULKAN_BACKEND) && GE_ENABLE_VULKAN_BACKEND
                return std::make_shared<VulkanFramebuffer>(spec);
#else
                return nullptr;
#endif
            case RenderAPI::WebGL2:
#if defined(GE_ENABLE_WEBGL2_BACKEND) && GE_ENABLE_WEBGL2_BACKEND
                return std::make_shared<WebGL2Framebuffer>(spec);
#else
                return nullptr;
#endif
        }

        return nullptr;
    }

} // namespace renderer
} // namespace ge
