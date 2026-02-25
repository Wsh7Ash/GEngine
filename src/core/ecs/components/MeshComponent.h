#pragma once

#include <memory>

namespace ge {
namespace renderer {
    class Mesh;
    class Shader;
}

namespace ecs {

/**
 * @brief Component that attaches a Mesh and a Shader to an entity.
 */
struct MeshComponent
{
    std::shared_ptr<renderer::Mesh>   MeshPtr   = nullptr;
    std::shared_ptr<renderer::Shader> ShaderPtr = nullptr;
};

} // namespace ecs
} // namespace ge
