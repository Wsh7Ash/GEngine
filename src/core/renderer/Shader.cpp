#include "Shader.h"
#include "RendererAPI.h"
#include "opengl/OpenGLShader.h"

#if defined(GE_ENABLE_WEBGL2_BACKEND) && GE_ENABLE_WEBGL2_BACKEND
#include "webgl2/WebGL2Shader.h"
#endif

#if defined(GE_ENABLE_DX11_BACKEND) && GE_ENABLE_DX11_BACKEND
#include "dx11/DX11Shader.h"
#endif

#if defined(GE_ENABLE_VULKAN_BACKEND) && GE_ENABLE_VULKAN_BACKEND
#include "vulkan/VulkanShader.h"
#endif

namespace ge {
namespace renderer {

    std::shared_ptr<Shader> Shader::Create(const std::string& filepath)
    {
        switch (RendererAPI::GetAPI())
        {
            case RenderAPI::None:    return nullptr;
            case RenderAPI::OpenGL:  return std::make_shared<OpenGLShader>(filepath);
            case RenderAPI::DX11:
#if defined(GE_ENABLE_DX11_BACKEND) && GE_ENABLE_DX11_BACKEND
                return std::make_shared<DX11Shader>(filepath);
#else
                return nullptr;
#endif
            case RenderAPI::Vulkan:
#if defined(GE_ENABLE_VULKAN_BACKEND) && GE_ENABLE_VULKAN_BACKEND
                return std::make_shared<VulkanShader>(filepath);
#else
                return nullptr;
#endif
            case RenderAPI::WebGL2:
#if defined(GE_ENABLE_WEBGL2_BACKEND) && GE_ENABLE_WEBGL2_BACKEND
                return std::make_shared<WebGL2Shader>(filepath);
#else
                return nullptr;
#endif
        }
        return nullptr;
    }

    std::shared_ptr<Shader> Shader::Create(const std::string& vertexPath, const std::string& fragmentPath)
    {
        switch (RendererAPI::GetAPI())
        {
            case RenderAPI::None:    return nullptr;
            case RenderAPI::OpenGL:  return std::make_shared<OpenGLShader>(vertexPath, fragmentPath);
            case RenderAPI::DX11:
#if defined(GE_ENABLE_DX11_BACKEND) && GE_ENABLE_DX11_BACKEND
                return std::make_shared<DX11Shader>(vertexPath, fragmentPath);
#else
                return nullptr;
#endif
            case RenderAPI::Vulkan:
#if defined(GE_ENABLE_VULKAN_BACKEND) && GE_ENABLE_VULKAN_BACKEND
                return std::make_shared<VulkanShader>(vertexPath, fragmentPath);
#else
                return nullptr;
#endif
            case RenderAPI::WebGL2:
#if defined(GE_ENABLE_WEBGL2_BACKEND) && GE_ENABLE_WEBGL2_BACKEND
                return std::make_shared<WebGL2Shader>(vertexPath, fragmentPath);
#else
                return nullptr;
#endif
        }

        return nullptr;
    }

} // namespace renderer
} // namespace ge
