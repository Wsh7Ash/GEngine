#pragma once

#include "../../renderer/Texture.h"
#include "../../math/VecTypes.h"
#include "../../gameplay/GridPathfinder.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace ge {
namespace ecs {

struct TilemapLayer {
    std::string Name = "Layer";
    std::vector<int32_t> Tiles;
    bool Visible = true;
    bool CollisionLayer = false;
    float ZOffset = 0.0f;
};

struct TilemapComponent {
    std::string TilesetTexturePath;
    std::shared_ptr<renderer::Texture> TilesetTexture = nullptr;
    int Width = 0;
    int Height = 0;
    int TileWidth = 16;
    int TileHeight = 16;
    int TilesPerRow = 1;
    float PixelsPerUnit = 16.0f;
    int ChunkWidth = 16;
    int ChunkHeight = 16;
    std::vector<Math::Vec4f> TilePalette;
    std::vector<TilemapLayer> Layers;
    gameplay::GridPathData Navigation;
    bool AutoBuildNavigation = true;
};

} // namespace ecs
} // namespace ge
