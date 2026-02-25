#include "Mesh.h"
#include "RendererAPI.h"
#include "opengl/OpenGLMesh.h"
// #include "dx11/DX11Mesh.h" // Placeholder

namespace ge {
namespace renderer {

    std::shared_ptr<Mesh> Mesh::Create(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
    {
        switch (RendererAPI::GetAPI())
        {
            case RenderAPI::None:    return nullptr;
            case RenderAPI::OpenGL:  return std::make_shared<OpenGLMesh>(vertices, indices);
            case RenderAPI::DX11:    return nullptr; // TODO
        }

        return nullptr;
    }

    std::shared_ptr<Mesh> Mesh::CreateCube()
    {
        std::vector<Vertex> vertices = {
            // Front face
            {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}},
            {{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}},
            {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
            {{-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}},
            // Back face
            {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}},
            {{ 0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}},
            {{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}},
            {{-0.5f,  0.5f, -0.5f}, {0.1f, 0.1f, 0.1f}}
        };

        std::vector<uint32_t> indices = {
            0, 1, 2, 2, 3, 0,
            1, 5, 6, 6, 2, 1,
            7, 6, 5, 5, 4, 7,
            4, 0, 3, 3, 7, 4,
            4, 5, 1, 1, 0, 4,
            3, 2, 6, 6, 7, 3
        };

        return Create(vertices, indices);
    }

} // namespace renderer
} // namespace ge
