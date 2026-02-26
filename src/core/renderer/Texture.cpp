#include "Texture.h"
#include "RendererAPI.h"
#include "opengl/OpenGLTexture.h"
#include "dx11/DX11Texture.h"

namespace ge {
namespace renderer {

    std::shared_ptr<Texture> Texture::Create(const std::string& path)
    {
        switch (RendererAPI::GetAPI())
        {
            case RenderAPI::None:    return nullptr;
            case RenderAPI::OpenGL:  return std::make_shared<OpenGLTexture>(path);
            case RenderAPI::DX11:    return std::make_shared<DX11Texture>(path);
        }

        return nullptr;
    }

    std::shared_ptr<Texture> Texture::Create(uint32_t width, uint32_t height, void* data, uint32_t size)
    {
        switch (RendererAPI::GetAPI())
        {
            case RenderAPI::None:    return nullptr;
            case RenderAPI::OpenGL:  return std::make_shared<OpenGLTexture>(width, height, data, size);
            case RenderAPI::DX11:    return std::make_shared<DX11Texture>(width, height, data, size);
        }

        return nullptr;
    }

} // namespace renderer
} // namespace ge
