#include "ViewportPanel.h"
#include "../ecs/World.h"
#include "../ecs/components/TagComponent.h"
#include "../ecs/components/TransformComponent.h"
#include "../renderer/Renderer2D.h"
#include "../renderer/OrthographicCamera.h"
#include "../editor/EditorToolbar.h"
#include "../cmd/CommandHistory.h"
#include "../cmd/EntityCommands.h"
#include "../math/MathUtils.h"
#include <ImGuizmo.h>
#include <imgui.h>
#include <algorithm>
#include <cmath>

namespace ge {
namespace editor {

namespace {

constexpr float kEditorPixelsPerUnit = 16.0f;
constexpr float kEditorZoom = 1.0f;

GizmoMode ToGizmoMode(int mode) {
  switch (mode) {
    case 1:
      return GizmoMode::Rotate;
    case 2:
      return GizmoMode::Scale;
    default:
      return GizmoMode::Translate;
  }
}

ImGuizmo::OPERATION ToImGuizmoOperation(GizmoMode mode) {
  switch (mode) {
    case GizmoMode::Rotate:
      return ImGuizmo::ROTATE;
    case GizmoMode::Scale:
      return ImGuizmo::SCALE;
    case GizmoMode::Translate:
    default:
      return ImGuizmo::TRANSLATE;
  }
}

Math::Vec3f ExtractScale(const Math::Mat4f& transform) {
  auto columnLength = [](const Math::Vec4f& column) {
    return std::sqrt(column.x * column.x + column.y * column.y + column.z * column.z);
  };

  return {
      (std::max)(columnLength(transform.cols[0]), 0.0001f),
      (std::max)(columnLength(transform.cols[1]), 0.0001f),
      (std::max)(columnLength(transform.cols[2]), 0.0001f)};
}

Math::Quatf ExtractRotation(const Math::Mat4f& transform, const Math::Vec3f& scale) {
  Math::Mat4f rotationMatrix = transform;
  rotationMatrix.cols[3] = {0.0f, 0.0f, 0.0f, 1.0f};

  rotationMatrix.cols[0].x /= scale.x;
  rotationMatrix.cols[0].y /= scale.x;
  rotationMatrix.cols[0].z /= scale.x;
  rotationMatrix.cols[0].w = 0.0f;

  rotationMatrix.cols[1].x /= scale.y;
  rotationMatrix.cols[1].y /= scale.y;
  rotationMatrix.cols[1].z /= scale.y;
  rotationMatrix.cols[1].w = 0.0f;

  rotationMatrix.cols[2].x /= scale.z;
  rotationMatrix.cols[2].y /= scale.z;
  rotationMatrix.cols[2].z /= scale.z;
  rotationMatrix.cols[2].w = 0.0f;

  return Math::Quatf::FromMat4x4(rotationMatrix);
}

bool HasTransformChanged(const Math::Vec3f& oldPosition,
                         const Math::Vec3f& newPosition,
                         const Math::Quatf& oldRotation,
                         const Math::Quatf& newRotation,
                         const Math::Vec3f& oldScale,
                         const Math::Vec3f& newScale) {
  return !oldPosition.ApproxEqual(newPosition, 0.0001f) ||
         !oldRotation.RotationEqual(newRotation, 0.0001f) ||
         !oldScale.ApproxEqual(newScale, 0.0001f);
}

Math::Vec2f GetViewportWorldSize(const Math::Vec2f& viewportSize) {
  return {
      viewportSize.x / (kEditorPixelsPerUnit * kEditorZoom),
      viewportSize.y / (kEditorPixelsPerUnit * kEditorZoom)};
}

renderer::OrthographicCamera BuildEditorCamera(const Math::Vec2f& viewportSize,
                                               const Math::Vec2f& cameraPosition) {
  renderer::OrthographicCamera camera(-1.0f, 1.0f, -1.0f, 1.0f);
  camera.SetPixelPerfectEnabled(true);
  camera.SetPixelSnap(true);
  camera.SetPixelsPerUnit(kEditorPixelsPerUnit);
  camera.SetZoom(kEditorZoom);
  camera.SetViewportSize(viewportSize.x, viewportSize.y);
  camera.SetPosition({cameraPosition.x, cameraPosition.y, 0.0f});
  return camera;
}

ImVec2 WorldToScreen(const Math::Vec3f& worldPosition,
                     const Math::Vec2f viewportBounds[2],
                     const Math::Vec2f& viewportSize,
                     const Math::Vec2f& cameraPosition) {
  const Math::Vec2f worldSize = GetViewportWorldSize(viewportSize);
  const float left = cameraPosition.x - worldSize.x * 0.5f;
  const float bottom = cameraPosition.y - worldSize.y * 0.5f;
  const float normalizedX = (worldPosition.x - left) / worldSize.x;
  const float normalizedY = (worldPosition.y - bottom) / worldSize.y;

  return {
      viewportBounds[0].x + normalizedX * viewportSize.x,
      viewportBounds[0].y + (1.0f - normalizedY) * viewportSize.y};
}

void DrawGridOverlay(ImDrawList* drawList,
                     const Math::Vec2f viewportBounds[2],
                     const Math::Vec2f& viewportSize,
                     const Math::Vec2f& cameraPosition) {
  if (!drawList || viewportSize.x <= 0.0f || viewportSize.y <= 0.0f) {
    return;
  }

  const Math::Vec2f worldSize = GetViewportWorldSize(viewportSize);
  const float left = cameraPosition.x - worldSize.x * 0.5f;
  const float right = cameraPosition.x + worldSize.x * 0.5f;
  const float bottom = cameraPosition.y - worldSize.y * 0.5f;
  const float top = cameraPosition.y + worldSize.y * 0.5f;

  drawList->PushClipRect(ImVec2(viewportBounds[0].x, viewportBounds[0].y),
                         ImVec2(viewportBounds[1].x, viewportBounds[1].y), true);

  for (int x = (int)std::floor(left); x <= (int)std::ceil(right); ++x) {
    const bool isAxis = (x == 0);
    const bool isMajor = (x % 4 == 0);
    const ImU32 color = isAxis
                            ? IM_COL32(70, 170, 220, 200)
                            : (isMajor ? IM_COL32(95, 95, 100, 130)
                                       : IM_COL32(60, 60, 64, 90));
    const ImVec2 start = WorldToScreen({(float)x, bottom, 0.0f}, viewportBounds,
                                       viewportSize, cameraPosition);
    const ImVec2 end = WorldToScreen({(float)x, top, 0.0f}, viewportBounds,
                                     viewportSize, cameraPosition);
    drawList->AddLine(start, end, color, isAxis ? 1.8f : 1.0f);
  }

  for (int y = (int)std::floor(bottom); y <= (int)std::ceil(top); ++y) {
    const bool isAxis = (y == 0);
    const bool isMajor = (y % 4 == 0);
    const ImU32 color = isAxis
                            ? IM_COL32(220, 120, 90, 200)
                            : (isMajor ? IM_COL32(95, 95, 100, 130)
                                       : IM_COL32(60, 60, 64, 90));
    const ImVec2 start = WorldToScreen({left, (float)y, 0.0f}, viewportBounds,
                                       viewportSize, cameraPosition);
    const ImVec2 end = WorldToScreen({right, (float)y, 0.0f}, viewportBounds,
                                     viewportSize, cameraPosition);
    drawList->AddLine(start, end, color, isAxis ? 1.8f : 1.0f);
  }

  drawList->PopClipRect();
}

} // namespace

ViewportPanel::ViewportPanel(const std::string& name, bool isGameView)
    : name_(name), isGameView_(isGameView) {
  renderer::FramebufferSpecification spec;
  spec.Width = 1280;
  spec.Height = 720;
  spec.Attachments = {
    renderer::FramebufferTextureFormat::RGBA8,
    renderer::FramebufferTextureFormat::RED_INTEGER,
    renderer::FramebufferTextureFormat::Depth
  };
  framebuffer_ = renderer::Framebuffer::Create(spec);

  precisionTool_.Initialize();
  precisionTool_.SetMode(GizmoMode::Translate);
  precisionTool_.EnableSnap(false);
  gizmo_.Initialize();
  gizmo_.SetMode(GizmoMode::Translate);
  gizmo_.SetSpace(GizmoSpace::Local);
}

void ViewportPanel::OnImGuiRender() {
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
  isVisible_ = ImGui::Begin(name_.c_str());
  if (!isVisible_) {
      ImGui::End();
      ImGui::PopStyleVar();
      return;
  }

  auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
  auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
  auto viewportOffset = ImGui::GetWindowPos();
  viewportBounds_[0] = {viewportMinRegion.x + viewportOffset.x,
                        viewportMinRegion.y + viewportOffset.y};
  viewportBounds_[1] = {viewportMaxRegion.x + viewportOffset.x,
                        viewportMaxRegion.y + viewportOffset.y};

  isFocused_ = ImGui::IsWindowFocused();
  isHovered_ = ImGui::IsWindowHovered();

  if (isHovered_)
    ImGui::SetWindowFocus();

  ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
  if ((std::abs(viewportSize_.x - viewportPanelSize.x) > 1.0f ||
       std::abs(viewportSize_.y - viewportPanelSize.y) > 1.0f) &&
      viewportPanelSize.x > 0 && viewportPanelSize.y > 0) {
    viewportSize_ = Math::Vec2f(viewportPanelSize.x, viewportPanelSize.y);
    framebuffer_->Resize((uint32_t)viewportSize_.x, (uint32_t)viewportSize_.y);
  }

  // Viewport Panning
  if (!isGameView_ && isHovered_ && ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
    ImVec2 delta = ImGui::GetIO().MouseDelta;
    cameraPosition_.x -= delta.x / (kEditorPixelsPerUnit * kEditorZoom);
    cameraPosition_.y += delta.y / (kEditorPixelsPerUnit * kEditorZoom);
  }

  uint32_t textureID = resultTexture_ != 0 ? resultTexture_ : framebuffer_->GetColorAttachmentRendererID();
  ImGui::Image((void *)(uintptr_t)textureID,
               ImVec2{viewportSize_.x, viewportSize_.y}, ImVec2{0, 1},
               ImVec2{1, 0});

  if (!isGameView_) {
      precisionTool_.Update();

      // ── Viewport Toolbar ──
      ImGui::SetCursorPos(ImVec2(10, 30));

      // Gizmo Mode Buttons
      auto gizmoButton = [&](const char* label, int mode) {
        bool active = (gizmoMode_ == mode);
        if (active) {
          ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.00f, 0.47f, 0.84f, 1.00f));
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.00f, 0.55f, 0.95f, 1.00f));
        } else {
          ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.20f, 0.22f, 0.80f));
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.30f, 0.35f, 0.90f));
        }
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.00f, 0.40f, 0.75f, 1.00f));
        if (ImGui::Button(label, ImVec2(28, 22))) {
          gizmoMode_ = mode;
          gizmo_.SetMode(ToGizmoMode(gizmoMode_));
          precisionTool_.SetMode(ToGizmoMode(gizmoMode_));
        }
        ImGui::PopStyleColor(3);
      };

      gizmoButton("W", 0); ImGui::SameLine();
      gizmoButton("E", 1); ImGui::SameLine();
      gizmoButton("R", 2); ImGui::SameLine();
      ImGui::Spacing(); ImGui::SameLine();

      // Keyboard shortcuts for gizmo modes
      if (isFocused_ && !ImGui::GetIO().WantTextInput) {
        if (ImGui::IsKeyPressed(ImGuiKey_W)) gizmoMode_ = 0;
        if (ImGui::IsKeyPressed(ImGuiKey_E)) gizmoMode_ = 1;
        if (ImGui::IsKeyPressed(ImGuiKey_R)) gizmoMode_ = 2;
      }
      gizmo_.SetMode(ToGizmoMode(gizmoMode_));
      precisionTool_.SetMode(ToGizmoMode(gizmoMode_));

      ImGui::Checkbox("Grid", &showGrid_);
      ImGui::SameLine();
      if (ImGui::Checkbox("Snap", &snap_)) {
        precisionTool_.EnableSnap(snap_);
      }
      if (snap_) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(50);
        if (ImGui::DragFloat("##SnapValue", &snapValue_, 0.05f, 0.05f, 5.0f, "%.2f")) {
          precisionTool_.SetPositionSnap(snapValue_);
          precisionTool_.SetScaleSnap(snapValue_);
        }
      }
      precisionTool_.EnableSnap(snap_);
      precisionTool_.SetPositionSnap(snapValue_);
      precisionTool_.SetScaleSnap(snapValue_);
      ImGui::SameLine(ImGui::GetWindowWidth() - 80.0f);
      ImGui::SetNextItemWidth(60);
      ImGui::ColorEdit4("##Clear", &clearColor_.x,
                        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);

      if (showGrid_ && viewportSize_.x > 0 && viewportSize_.y > 0) {
        DrawGridOverlay(ImGui::GetWindowDrawList(), viewportBounds_, viewportSize_,
                        cameraPosition_);
      }
    
      // Mouse Picking
      auto [mx, my] = ImGui::GetMousePos();
      mx -= viewportBounds_[0].x;
      my -= viewportBounds_[0].y;
      Math::Vec2f viewportSizeNorm = viewportBounds_[1] - viewportBounds_[0];
      my = viewportSizeNorm.y - my; // Invert Y for OpenGL
    
      int mouseX = (int)mx;
      int mouseY = (int)my;
    
      if (mouseX >= 0 && mouseY >= 0 && mouseX < (int)viewportSizeNorm.x &&
          mouseY < (int)viewportSizeNorm.y) {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGuizmo::IsOver()) {
          int pixelData = framebuffer_->ReadPixel(1, mouseX, mouseY);
          if (pixelData == -1) {
            EditorToolbar::ClearSelection();
          } else {
            ecs::Entity pickedEntity = ecs::INVALID_ENTITY;
            if (sceneContext_) {
              pickedEntity =
                  sceneContext_->ResolveEntityByIndex((uint32_t)pixelData);
            }
            EditorToolbar::SetSelectedEntity(pickedEntity);
          }
        }
      }
    
      // Gizmos
      ecs::Entity selectedEntity = EditorToolbar::GetSelectedEntity();
      if (selectedEntity && sceneContext_ &&
          sceneContext_->IsAlive(selectedEntity) &&
          sceneContext_->HasComponent<ecs::TransformComponent>(selectedEntity)) {
        ImGuizmo::SetOrthographic(true);
        ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
        ImGuizmo::SetRect(viewportBounds_[0].x, viewportBounds_[0].y,
                          viewportBounds_[1].x - viewportBounds_[0].x,
                          viewportBounds_[1].y - viewportBounds_[0].y);
    
        const auto camera = BuildEditorCamera(viewportSize_, cameraPosition_);
        float view[16];
        float projection[16];
        std::copy_n(camera.GetViewMatrix().Data(), 16, view);
        std::copy_n(camera.GetProjectionMatrix().Data(), 16, projection);
    
        auto &tc =
            sceneContext_->GetComponent<ecs::TransformComponent>(selectedEntity);
        gizmo_.SetCamera(camera.GetViewMatrix(), camera.GetProjectionMatrix(),
                         viewportSize_.y > 0.0f ? viewportSize_.x / viewportSize_.y : 1.0f);
        gizmo_.SetTransform(tc.position, tc.rotation, tc.scale);
        gizmo_.SetScreenPosition(
            {(viewportBounds_[0].x + viewportBounds_[1].x) * 0.5f,
             (viewportBounds_[0].y + viewportBounds_[1].y) * 0.5f});
        Math::Mat4f transform = Math::Mat4f::Translate(tc.position) *
                                tc.rotation.ToMat4x4() *
                                Math::Mat4f::Scale(tc.scale);
    
        float snapValues[3] = {
            precisionTool_.GetSnapSettings().GetEffectivePositionSnap(),
            precisionTool_.GetSnapSettings().GetEffectivePositionSnap(),
            precisionTool_.GetSnapSettings().GetEffectivePositionSnap()};

        const GizmoMode gizmoMode = ToGizmoMode(gizmoMode_);
        if (gizmoMode == GizmoMode::Rotate) {
          snapValues[0] = precisionTool_.GetSnapSettings().GetEffectiveRotationSnap();
          snapValues[1] = snapValues[0];
          snapValues[2] = snapValues[0];
        } else if (gizmoMode == GizmoMode::Scale) {
          snapValues[0] = precisionTool_.GetSnapSettings().GetEffectiveScaleSnap();
          snapValues[1] = snapValues[0];
          snapValues[2] = snapValues[0];
        }

        ImGuizmo::Manipulate(view, projection, ToImGuizmoOperation(gizmoMode), ImGuizmo::LOCAL,
                             transform.Data(), nullptr,
                             precisionTool_.IsSnapEnabled() ? snapValues : nullptr);
    
        if (ImGuizmo::IsUsing()) {
          if (!isUsingGizmo_) {
            // Started using gizmo - capture starting state
            startingPosition_ = tc.position;
            startingRotation_ = tc.rotation;
            startingScale_ = tc.scale;
            isUsingGizmo_ = true;
            gizmo_.HandleMouseDown({mx, my}, 0);
            precisionTool_.OnGizmoDragBegin();
          }

          Math::Vec3f newPosition = {transform.cols[3].x, transform.cols[3].y,
                                     transform.cols[3].z};
          Math::Vec3f newScale = ExtractScale(transform);
          Math::Quatf newRotation = ExtractRotation(transform, newScale);

          if (gizmoMode == GizmoMode::Translate && precisionTool_.IsSnapEnabled()) {
            newPosition = precisionTool_.SnapPosition(newPosition);
          }

          if (gizmoMode == GizmoMode::Rotate) {
            Math::Vec3f euler = newRotation.ToEuler();
            Math::Vec3f degrees = {
                0.0f,
                0.0f,
                Math::RadiansToDegrees(euler.z)};

            if (precisionTool_.IsSnapEnabled()) {
              degrees = precisionTool_.SnapRotation(degrees);
            }

            newRotation = Math::Quatf::FromEuler(
                Math::DegreesToRadians(degrees.x),
                Math::DegreesToRadians(degrees.y),
                Math::DegreesToRadians(degrees.z));
          }

          if (gizmoMode == GizmoMode::Scale && precisionTool_.IsSnapEnabled()) {
            newScale = precisionTool_.SnapScale(newScale);
          }

          tc.position = newPosition;
          tc.rotation = newRotation;
          tc.scale = newScale;
          gizmo_.SetTransform(tc.position, tc.rotation, tc.scale);
          gizmo_.HandleMouseMove({mx, my});
        } else if (isUsingGizmo_) {
          // Just stopped using gizmo - push final command
          if (HasTransformChanged(startingPosition_, tc.position,
                                  startingRotation_, tc.rotation,
                                  startingScale_, tc.scale)) {
            cmd::CommandHistory::PushCommand(std::make_unique<cmd::CommandChangeTransform>(
                *sceneContext_, selectedEntity,
                startingPosition_, tc.position,
                startingRotation_, tc.rotation,
                startingScale_, tc.scale
            ));
          }
          gizmo_.HandleMouseUp(0);
          precisionTool_.OnGizmoDragEnd();
          isUsingGizmo_ = false;
        }
      }
    
      // Render viewport-space indicators for specialized entities (SpawnPoints,
      // etc.)
      if (sceneContext_) {
        auto drawList = ImGui::GetWindowDrawList();
        // Iterate through all entities that have a TagComponent
        for (auto e : sceneContext_->Query<ecs::TagComponent>()) {
          if (sceneContext_->IsAlive(e) &&
              sceneContext_->HasComponent<ecs::TransformComponent>(e)) {
            auto &tag = sceneContext_->GetComponent<ecs::TagComponent>(e).tag;
            if (tag == "SpawnPoint") {
              auto &tc = sceneContext_->GetComponent<ecs::TransformComponent>(e);
              ImVec2 pos =
                  WorldToScreen(tc.position, viewportBounds_, viewportSize_,
                                cameraPosition_);
    
              drawList->AddCircleFilled(pos, 10.0f, IM_COL32(0, 255, 0, 200));
              drawList->AddText(ImVec2(pos.x + 12, pos.y - 6),
                                IM_COL32(255, 255, 255, 255), "Spawn Point");
            }
          }
        }
      }
  }

  ImGui::End();
  ImGui::PopStyleVar();
}

} // namespace editor
} // namespace ge
