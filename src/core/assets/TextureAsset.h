#pragma once

#include "Asset.h"
#include "../renderer/Texture.h"
#include <memory>

namespace ge {
namespace assets {

    /**
     * @brief Wrapper for Texture assets.
     */
    class TextureAsset : public Asset
    {
    public:
        std::shared_ptr<renderer::Texture> Texture;

        TextureAsset(std::shared_ptr<renderer::Texture> texture)
            : Texture(texture) {}

        AssetType GetType() const override { return AssetType::Texture; }
    };

} // namespace assets
} // namespace ge
