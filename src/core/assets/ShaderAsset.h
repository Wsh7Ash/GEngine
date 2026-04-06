#pragma once

#include "Asset.h"
#include "../renderer/Shader.h"
#include <memory>

namespace ge {
namespace assets {

    /**
     * @brief Asset wrapper for a Shader.
     */
    class ShaderAsset : public Asset
    {
    public:
        std::shared_ptr<renderer::Shader> Shader;

        ShaderAsset(std::shared_ptr<renderer::Shader> shader)
            : Shader(shader) {}

        virtual AssetType GetType() const override { return AssetType::Shader; }
    };

} // namespace assets
} // namespace ge