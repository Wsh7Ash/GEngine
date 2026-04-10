#pragma once

#include "Asset.h"
#include "../renderer/Texture.h"
#include <memory>
#include <string>

namespace ge {
namespace assets {

class TilesetAsset : public Asset
{
public:
    std::string TexturePath;
    std::shared_ptr<renderer::Texture> Texture;
    int TileWidth = 16;
    int TileHeight = 16;
    int TilesPerRow = 1;

    AssetType GetType() const override { return AssetType::Tileset; }
};

} // namespace assets
} // namespace ge
