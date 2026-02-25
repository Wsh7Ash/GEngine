#pragma once

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
    renderer::Mesh*   MeshPtr   = nullptr;
    renderer::Shader* ShaderPtr = nullptr;
};

} // namespace ecs
} // namespace ge
