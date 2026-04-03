#pragma once

#include "../math/VecTypes.h"
#include "../math/quaternion.h"
#include "../renderer/Framebuffer.h"
#include "TransformGizmo.h"
#include "PrecisionEditTool.h"
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
  ViewportPanel(const std::string& name, bool isGameView);
  ~ViewportPanel() = default;

  void OnImGuiRender();

  void SetContext(ecs::World &world) { sceneContext_ = &world; }
  std::shared_ptr<renderer::Framebuffer> GetFramebuffer() {
    return framebuffer_;
  }
  Math::Vec4f GetClearColor() const { return clearColor_; }
  bool IsVisible() const { return isVisible_; }
  Math::Vec2f GetSize() const { return viewportSize_; }

  void SetResultTexture(uint32_t id) { resultTexture_ = id; }

private:
  std::string name_;
  bool isGameView_;
  bool isVisible_ = true;

  std::shared_ptr<renderer::Framebuffer> framebuffer_;
  uint32_t resultTexture_ = 0;
  ecs::World *sceneContext_ = nullptr;

  bool isFocused_ = false;
  bool isHovered_ = false;
  Math::Vec2f viewportSize_ = {0, 0};
  Math::Vec2f viewportBounds_[2];
  Math::Vec2f cameraPosition_ = {0, 0};
  bool showGrid_ = true;
  bool snap_ = false;
  float snapValue_ = 0.5f;
  int gizmoMode_ = 0; // 0=Translate, 1=Rotate, 2=Scale
  Math::Vec4f clearColor_ = {0.1f, 0.1f, 0.11f, 1.0f};

  // Undo/Redo state tracking
  Math::Vec3f startingPosition_ = {0, 0, 0};
  Math::Quatf startingRotation_ = Math::Quatf::Identity();
  Math::Vec3f startingScale_ = {1, 1, 1};
  bool isUsingGizmo_ = false;

  // Transform gizmo
  TransformGizmo gizmo_;
  PrecisionEditTool precisionTool_;
  bool gizmoEnabled_ = true;
  int currentGizmoMode_ = 0;
  int currentGizmoSpace_ = 0;
};

} // namespace editor
} // namespace ge
