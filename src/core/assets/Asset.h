#pragma once

#include "../uuid/UUID.h"
#include <string>

namespace ge {
namespace assets {

    using AssetHandle = ge::UUID;

    enum class AssetType : uint16_t
    {
        None = 0,
        Scene,
        Texture,
        Tileset,
        Tilemap,
        Mesh,
        Shader,
        Material,
        Audio,
        Font,
        Script
    };

    /**
     * @brief Base class for all engine assets.
     */
    class Asset
    {
    public:
        AssetHandle Handle;

        virtual ~Asset() = default;
        virtual AssetType GetType() const = 0;
    };

} // namespace assets
} // namespace ge
