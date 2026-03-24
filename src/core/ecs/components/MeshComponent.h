#pragma once

#include "../../math/VecTypes.h"
#include <string>

namespace ge {
namespace renderer {
    class Mesh;
    class Material;
}

namespace ecs {

/**
 * @brief Component that attaches a Mesh and a Material to an entity.
 */
struct MeshComponent
{
    std::shared_ptr<renderer::Mesh>     MeshPtr     = nullptr;
    std::shared_ptr<renderer::Material> MaterialPtr = nullptr;

    std::string MeshPath = "";
    
    // PBR Properties
    Math::Vec3f AlbedoColor = {1.0f, 1.0f, 1.0f};
    float       Metallic    = 0.0f;
    float       Roughness   = 0.5f;

    // Texture Paths
    std::string AlbedoPath    = "";
    std::string NormalPath    = "";
    std::string MetallicPath  = "";
    std::string RoughnessPath = "";
    std::string AOPath        = "";
};

} // namespace ecs
} // namespace ge
