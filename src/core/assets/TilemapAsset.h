#pragma once

#include "Asset.h"
#include "../ecs/components/TilemapComponent.h"

namespace ge {
namespace assets {

class TilemapAsset : public Asset
{
public:
    ecs::TilemapComponent Tilemap;

    AssetType GetType() const override { return AssetType::Tilemap; }
};

} // namespace assets
} // namespace ge
