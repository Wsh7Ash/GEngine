#pragma once

#include "../../math/VecTypes.h"
#include "../../renderer/Texture.h"
#include <memory>
#include <string>

namespace ge {
namespace ecs {

/**
 * @brief Component for rendering 2D sprites.
 */
struct SpriteComponent {
  std::shared_ptr<renderer::Texture> texture = nullptr;
  std::string TexturePath;
  Math::Vec4f color = {1.0f, 1.0f, 1.0f, 1.0f}; // Tint color
  Math::Vec2f tiling = {1.0f, 1.0f};
  Math::Vec2f Pivot = {0.5f, 0.5f};
  bool FlipX = false;
  bool FlipY = false;

  // Ordering
  int SortingLayer = 0;
  int OrderInLayer = 0;
  bool YSort = false;

  // Pixel art sizing
  float PixelsPerUnit = 16.0f;
  bool UseSourceSize = false;

  // Atlas region (normalized UV bounds: min.x, min.y, max.x, max.y)
  bool UseAtlasRegion = false;
  Math::Vec4f AtlasRegion = {0.0f, 0.0f, 1.0f, 1.0f};

  // Animation
  bool isAnimated = false;
  int framesX = 1;
  int framesY = 1;
  int currentFrame = 0;
  float frameTime = 0.1f; // Seconds per frame
  float elapsedTime = 0.0f;
};

} // namespace ecs
} // namespace ge
