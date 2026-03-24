#pragma once

#include "../../math/VecTypes.h"
#include "../../assets/Asset.h"
#include <string>
#include <memory>

namespace ge {
namespace renderer {
    class Model;
}

namespace ecs {

/**
 * @brief Component that attaches a 3D Model (possibly with bones/skinning) to an entity.
 */
struct ModelComponent
{
    assets::AssetHandle ModelHandle = 0;
    std::shared_ptr<renderer::Model> ModelPtr = nullptr;
    
    // PBR Properties (Applied to all sub-meshes for now)
    Math::Vec3f AlbedoColor = {1.0f, 1.0f, 1.0f};
    float       Metallic    = 0.0f;
    float       Roughness   = 0.5f;

    std::string ModelPath = "";
};

} // namespace ecs
} // namespace ge
