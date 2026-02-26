#pragma once

#include "../../renderer/Texture.h"
#include "../../math/VecTypes.h"
#include <memory>

namespace ge {
namespace ecs {

    /**
     * @brief Component for rendering 2D sprites.
     */
    struct SpriteComponent
    {
        std::shared_ptr<renderer::Texture> TexturePtr = nullptr;
        Math::Vec4f Color = { 1.0f, 1.0f, 1.0f, 1.0f }; // Tint color
        Math::Vec2f Tiling = { 1.0f, 1.0f };
    };

} // namespace ecs
} // namespace ge
