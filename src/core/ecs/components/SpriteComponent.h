#pragma once

#include "../../math/VecTypes.h"
#include "../../renderer/Texture.h"
#include <memory>

namespace ge {
namespace ecs {

/**
 * @brief Component for rendering 2D sprites.
 */
struct SpriteComponent {
  std::shared_ptr<renderer::Texture> texture = nullptr;
  Math::Vec4f color = {1.0f, 1.0f, 1.0f, 1.0f}; // Tint color
  Math::Vec2f tiling = {1.0f, 1.0f};
  bool FlipX = false;
  bool FlipY = false;

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
