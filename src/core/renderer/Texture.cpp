#include "Texture.h"
#include "RendererAPI.h"
#include "opengl/OpenGLTexture.h"

#if defined(GE_ENABLE_WEBGL2_BACKEND) && GE_ENABLE_WEBGL2_BACKEND
#include "webgl2/WebGL2Texture.h"
#endif

#if defined(GE_ENABLE_DX11_BACKEND) && GE_ENABLE_DX11_BACKEND
#include "dx11/DX11Texture.h"
#endif

namespace ge {
namespace renderer {

    std::shared_ptr<Texture> Texture::Create(const std::string& path, bool hdr)
    {
        return Create(path, TextureSpecification{}, hdr);
    }

    std::shared_ptr<Texture> Texture::Create(const std::string& path, const TextureSpecification& specification, bool hdr)
    {
        switch (RendererAPI::GetAPI())
        {
            case RenderAPI::None:    return nullptr;
            case RenderAPI::OpenGL:  return std::make_shared<OpenGLTexture>(path, specification);
            case RenderAPI::DX11:
#if defined(GE_ENABLE_DX11_BACKEND) && GE_ENABLE_DX11_BACKEND
                return std::make_shared<DX11Texture>(path);
#else
                return nullptr;
#endif
            case RenderAPI::Vulkan:
                return nullptr;
            case RenderAPI::WebGL2:
#if defined(GE_ENABLE_WEBGL2_BACKEND) && GE_ENABLE_WEBGL2_BACKEND
                return std::make_shared<WebGL2Texture>(path);
#else
                return nullptr;
#endif
        }

        return nullptr;
    }

    std::shared_ptr<Texture> Texture::Create(uint32_t width, uint32_t height, void* data, uint32_t size, bool hdr)
    {
        return Create(width, height, data, size, TextureSpecification{}, hdr);
    }

    std::shared_ptr<Texture> Texture::Create(uint32_t width, uint32_t height, void* data, uint32_t size, const TextureSpecification& specification, bool hdr)
    {
        (void)hdr;
        switch (RendererAPI::GetAPI())
        {
            case RenderAPI::None:    return nullptr;
            case RenderAPI::OpenGL:  return std::make_shared<OpenGLTexture>(width, height, data, size, specification);
            case RenderAPI::DX11:
#if defined(GE_ENABLE_DX11_BACKEND) && GE_ENABLE_DX11_BACKEND
                return std::make_shared<DX11Texture>(width, height, data, size);
#else
                return nullptr;
#endif
            case RenderAPI::Vulkan:
                return nullptr;
            case RenderAPI::WebGL2:
#if defined(GE_ENABLE_WEBGL2_BACKEND) && GE_ENABLE_WEBGL2_BACKEND
                return std::make_shared<WebGL2Texture>(width, height, data, size);
#else
                return nullptr;
#endif
        }

        return nullptr;
    }

} // namespace renderer
} // namespace ge
