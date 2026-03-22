#pragma once

#include "../../math/VecTypes.h"
#include "../../assets/Asset.h"

namespace ge {
namespace ecs {

    /**
     * @brief Component for rendering a sprite or color in screen space.
     */
    struct UIImageComponent
    {
        Math::Vec4f Color = { 1.0f, 1.0f, 1.0f, 1.0f };
        assets::AssetHandle TextureHandle = 0;
    };

} // namespace ecs
} // namespace ge
