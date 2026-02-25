#include "Shader.h"
#include "RendererAPI.h"
#include "opengl/OpenGLShader.h"
// #include "dx11/DX11Shader.h" // Placeholder

namespace ge {
namespace renderer {

    std::shared_ptr<Shader> Shader::Create(const std::string& vertexPath, const std::string& fragmentPath)
    {
        switch (RendererAPI::GetAPI())
        {
            case RenderAPI::None:    return nullptr;
            case RenderAPI::OpenGL:  return std::make_shared<OpenGLShader>(vertexPath, fragmentPath);
            case RenderAPI::DX11:    return nullptr; // TODO
        }

        return nullptr;
    }

} // namespace renderer
} // namespace ge
