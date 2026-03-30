#pragma once

#include "../../math/VecTypes.h"
#include "../../assets/MaterialGraphAsset.h"
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
    std::shared_ptr<assets::MaterialGraphAsset> MaterialGraphAsset = nullptr;

    std::string MeshPath = "";

    // LOD Support
    struct LODLevel {
        std::shared_ptr<renderer::Mesh> MeshPtr = nullptr;
        float DistanceThreshold = 0.0f; // Distance at which this LOD becomes active
    };
    std::vector<LODLevel> LODLevels;
    
    bool IsVisible = true; // Manual culling/occlusion hint
    
    // PBR Properties
    Math::Vec3f AlbedoColor = {1.0f, 1.0f, 1.0f};
    float       Metallic    = 0.0f;
    float       Roughness   = 0.5f;

    // Glass/Refraction Properties
    float       IOR = 1.0f;              // Index of Refraction (1.0=air, 1.5=glass, 1.33=water)
    float       Thickness = 0.0f;         // Material thickness for absorption
    Math::Vec3f TintColor = {1.0f, 1.0f, 1.0f}; // Glass tint/absorption color
    float       Translucency = 0.0f;      // 0=opaque, 1=fully translucent

    // Subsurface Scattering Properties
    Math::Vec3f SubsurfaceColor = {1.0f, 0.4f, 0.3f}; // SSS tint (skin-like default)
    float       SubsurfacePower = 12.0f;   // SSS intensity/distortion
    float       SSSIntensity = 0.0f;        // SSS blend amount

    // Texture Paths
    std::string AlbedoPath    = "";
    std::string NormalPath    = "";
    std::string MetallicPath  = "";
    std::string RoughnessPath = "";
    std::string AOPath        = "";
};

} // namespace ecs
} // namespace ge
