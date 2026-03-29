#pragma once

#include "../../math/VecTypes.h"
#include "../../math/Mat4x4.h"
#include "../../renderer/Texture.h"
#include <memory>

namespace ge {
namespace ecs {

/**
 * @brief Component for rendering decals via projective texturing.
 */
struct DecalComponent {
    std::shared_ptr<renderer::Texture> Albedo = nullptr;
    std::shared_ptr<renderer::Texture> Normal = nullptr;
    // Projection matrix that transforms world coordinates to [0,1] UV space
    Math::Mat4f Projection = Math::Mat4f::Identity();
    
    // Optional: fade distances for smooth transitions
    float FadeStart = 0.0f;
    float FadeEnd = 10.0f;
    
    bool Enabled = true;
};

} // namespace ecs
} // namespace ge