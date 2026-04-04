#include "Mesh.h"
#include "RendererAPI.h"
#include "opengl/OpenGLMesh.h"
#include "webgl2/WebGL2Mesh.h"
#include "dx11/DX11Mesh.h"
#include "vulkan/VulkanMesh.h"

namespace ge {
namespace renderer {

    std::shared_ptr<Mesh> Mesh::Create(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
    {
        switch (RendererAPI::GetAPI())
        {
            case RenderAPI::None:    return nullptr;
            case RenderAPI::OpenGL:  return std::make_shared<OpenGLMesh>(vertices, indices);
            case RenderAPI::DX11:    return std::make_shared<DX11Mesh>(vertices, indices);
            case RenderAPI::Vulkan:  return std::make_shared<VulkanMesh>(vertices, indices);
            case RenderAPI::WebGL2:  return std::make_shared<WebGL2Mesh>(vertices, indices);
        }

        return nullptr;
    }

    std::shared_ptr<Mesh> Mesh::CreateDynamic(uint32_t maxVertices, uint32_t maxIndices)
    {
        switch (RendererAPI::GetAPI())
        {
            case RenderAPI::None:    return nullptr;
            case RenderAPI::OpenGL:  return std::make_shared<OpenGLMesh>(maxVertices, maxIndices);
            case RenderAPI::DX11:    return std::make_shared<DX11Mesh>(maxVertices, maxIndices);
            case RenderAPI::Vulkan:  return std::make_shared<VulkanMesh>(maxVertices, maxIndices);
            case RenderAPI::WebGL2:  return std::make_shared<WebGL2Mesh>();
        }

        return nullptr;
    }

    std::shared_ptr<Mesh> Mesh::CreateCube()
    {
        std::vector<Vertex> vertices = {
            // Front face (+Z) Normal={0,0,1}
            {{-0.5f, -0.5f,  0.5f}, {0.0f,0.0f,1.0f}, {1.0f,0.0f,0.0f}, {0.0f,1.0f,0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, 0.0f, 1.0f, -1},
            {{ 0.5f, -0.5f,  0.5f}, {0.0f,0.0f,1.0f}, {1.0f,0.0f,0.0f}, {0.0f,1.0f,0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}, 0.0f, 1.0f, -1},
            {{ 0.5f,  0.5f,  0.5f}, {0.0f,0.0f,1.0f}, {1.0f,0.0f,0.0f}, {0.0f,1.0f,0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, 0.0f, 1.0f, -1},
            {{-0.5f,  0.5f,  0.5f}, {0.0f,0.0f,1.0f}, {1.0f,0.0f,0.0f}, {0.0f,1.0f,0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, 0.0f, 1.0f, -1},

            // Back face (-Z) Normal={0,0,-1}
            {{ 0.5f, -0.5f, -0.5f}, {0.0f,0.0f,-1.0f}, {-1.0f,0.0f,0.0f}, {0.0f,1.0f,0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, 0.0f, 1.0f, -1},
            {{-0.5f, -0.5f, -0.5f}, {0.0f,0.0f,-1.0f}, {-1.0f,0.0f,0.0f}, {0.0f,1.0f,0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}, 0.0f, 1.0f, -1},
            {{-0.5f,  0.5f, -0.5f}, {0.0f,0.0f,-1.0f}, {-1.0f,0.0f,0.0f}, {0.0f,1.0f,0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, 0.0f, 1.0f, -1},
            {{ 0.5f,  0.5f, -0.5f}, {0.0f,0.0f,-1.0f}, {-1.0f,0.0f,0.0f}, {0.0f,1.0f,0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, 0.0f, 1.0f, -1},

            // Left face (-X) Normal={-1,0,0}
            {{-0.5f, -0.5f, -0.5f}, {-1.0f,0.0f,0.0f}, {0.0f,0.0f,1.0f}, {0.0f,1.0f,0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, 0.0f, 1.0f, -1},
            {{-0.5f, -0.5f,  0.5f}, {-1.0f,0.0f,0.0f}, {0.0f,0.0f,1.0f}, {0.0f,1.0f,0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}, 0.0f, 1.0f, -1},
            {{-0.5f,  0.5f,  0.5f}, {-1.0f,0.0f,0.0f}, {0.0f,0.0f,1.0f}, {0.0f,1.0f,0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, 0.0f, 1.0f, -1},
            {{-0.5f,  0.5f, -0.5f}, {-1.0f,0.0f,0.0f}, {0.0f,0.0f,1.0f}, {0.0f,1.0f,0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, 0.0f, 1.0f, -1},

            // Right face (+X) Normal={1,0,0}
            {{ 0.5f, -0.5f,  0.5f}, {1.0f,0.0f,0.0f}, {0.0f,0.0f,-1.0f}, {0.0f,1.0f,0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, 0.0f, 1.0f, -1},
            {{ 0.5f, -0.5f, -0.5f}, {1.0f,0.0f,0.0f}, {0.0f,0.0f,-1.0f}, {0.0f,1.0f,0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}, 0.0f, 1.0f, -1},
            {{ 0.5f,  0.5f, -0.5f}, {1.0f,0.0f,0.0f}, {0.0f,0.0f,-1.0f}, {0.0f,1.0f,0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, 0.0f, 1.0f, -1},
            {{ 0.5f,  0.5f,  0.5f}, {1.0f,0.0f,0.0f}, {0.0f,0.0f,-1.0f}, {0.0f,1.0f,0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, 0.0f, 1.0f, -1},

            // Top face (+Y) Normal={0,1,0}
            {{-0.5f,  0.5f,  0.5f}, {0.0f,1.0f,0.0f}, {1.0f,0.0f,0.0f}, {0.0f,0.0f,-1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, 0.0f, 1.0f, -1},
            {{ 0.5f,  0.5f,  0.5f}, {0.0f,1.0f,0.0f}, {1.0f,0.0f,0.0f}, {0.0f,0.0f,-1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}, 0.0f, 1.0f, -1},
            {{ 0.5f,  0.5f, -0.5f}, {0.0f,1.0f,0.0f}, {1.0f,0.0f,0.0f}, {0.0f,0.0f,-1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, 0.0f, 1.0f, -1},
            {{-0.5f,  0.5f, -0.5f}, {0.0f,1.0f,0.0f}, {1.0f,0.0f,0.0f}, {0.0f,0.0f,-1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, 0.0f, 1.0f, -1},

            // Bottom face (-Y) Normal={0,-1,0}
            {{-0.5f, -0.5f, -0.5f}, {0.0f,-1.0f,0.0f}, {1.0f,0.0f,0.0f}, {0.0f,0.0f,1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, 0.0f, 1.0f, -1},
            {{ 0.5f, -0.5f, -0.5f}, {0.0f,-1.0f,0.0f}, {1.0f,0.0f,0.0f}, {0.0f,0.0f,1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}, 0.0f, 1.0f, -1},
            {{ 0.5f, -0.5f,  0.5f}, {0.0f,-1.0f,0.0f}, {1.0f,0.0f,0.0f}, {0.0f,0.0f,1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, 0.0f, 1.0f, -1},
            {{-0.5f, -0.5f,  0.5f}, {0.0f,-1.0f,0.0f}, {1.0f,0.0f,0.0f}, {0.0f,0.0f,1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, 0.0f, 1.0f, -1}
        };

        std::vector<uint32_t> indices = {
             0,  1,  2,  2,  3,  0,
             4,  5,  6,  6,  7,  4,
             8,  9, 10, 10, 11,  8,
            12, 13, 14, 14, 15, 12,
            16, 17, 18, 18, 19, 16,
            20, 21, 22, 22, 23, 20
        };

        return Create(vertices, indices);
    }

} // namespace renderer
} // namespace ge
