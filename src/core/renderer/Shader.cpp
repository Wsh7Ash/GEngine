#include "Shader.h"
#include "RendererAPI.h"
#include "opengl/OpenGLShader.h"
#include "webgl2/WebGL2Shader.h"
#include "dx11/DX11Shader.h"
#include "vulkan/VulkanShader.h"

namespace ge {
namespace renderer {

    std::shared_ptr<Shader> Shader::Create(const std::string& filepath)
    {
        switch (RendererAPI::GetAPI())
        {
            case RenderAPI::None:    return nullptr;
            case RenderAPI::OpenGL:  return std::make_shared<OpenGLShader>(filepath);
            case RenderAPI::DX11:    return std::make_shared<DX11Shader>(filepath);
            case RenderAPI::Vulkan:  return std::make_shared<VulkanShader>(filepath);
            case RenderAPI::WebGL2:  return std::make_shared<WebGL2Shader>(filepath);
        }
        return nullptr;
    }

    std::shared_ptr<Shader> Shader::Create(const std::string& vertexPath, const std::string& fragmentPath)
    {
        switch (RendererAPI::GetAPI())
        {
            case RenderAPI::None:    return nullptr;
            case RenderAPI::OpenGL:  return std::make_shared<OpenGLShader>(vertexPath, fragmentPath);
            case RenderAPI::DX11:    return std::make_shared<DX11Shader>(vertexPath, fragmentPath);
            case RenderAPI::Vulkan:  return std::make_shared<VulkanShader>(vertexPath, fragmentPath);
            case RenderAPI::WebGL2:  return std::make_shared<WebGL2Shader>(vertexPath, fragmentPath);
        }

        return nullptr;
    }

} // namespace renderer
} // namespace ge
