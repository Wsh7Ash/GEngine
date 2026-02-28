#pragma once

#include "../math/VecTypes.h"
#include "../renderer/Framebuffer.h"
#include <imgui.h>
#include <memory>


namespace ge {
namespace ecs {
class World;
}

namespace editor {

/**
 * @brief Panel that displays the rendered Framebuffer texture.
 */
class ViewportPanel {
public:
  ViewportPanel();
  ~ViewportPanel() = default;

  void OnImGuiRender();

  void SetContext(ecs::World &world) { sceneContext_ = &world; }
  std::shared_ptr<renderer::Framebuffer> GetFramebuffer() {
    return framebuffer_;
  }

private:
  std::shared_ptr<renderer::Framebuffer> framebuffer_;
  ecs::World *sceneContext_ = nullptr;

  bool isFocused_ = false;
  bool isHovered_ = false;
  Math::Vec2f viewportSize_ = {0, 0};
  Math::Vec2f viewportBounds_[2];
};

} // namespace editor
} // namespace ge
