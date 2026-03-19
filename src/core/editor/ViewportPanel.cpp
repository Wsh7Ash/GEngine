#include "ViewportPanel.h"
#include "../ecs/World.h"
#include "../ecs/components/TagComponent.h"
#include "../ecs/components/TransformComponent.h"
#include "../renderer/Renderer2D.h"
#include "../editor/EditorToolbar.h"
#include "../cmd/CommandHistory.h"
#include "../cmd/EntityCommands.h"
#include <ImGuizmo.h>
#include <imgui.h>

namespace ge {
namespace editor {

ViewportPanel::ViewportPanel(const std::string& name, bool isGameView)
    : name_(name), isGameView_(isGameView) {
  renderer::FramebufferSpecification spec;
  spec.Width = 1280;
  spec.Height = 720;
  framebuffer_ = renderer::Framebuffer::Create(spec);
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
    if (viewportSize_.x > 0)
      cameraPosition_.x -= delta.x * (2.0f / viewportSize_.x);
    if (viewportSize_.y > 0)
      cameraPosition_.y += delta.y * (2.0f / viewportSize_.y);
  }

  uint32_t textureID = framebuffer_->GetColorAttachmentRendererID();
  ImGui::Image((void *)(uintptr_t)textureID,
               ImVec2{viewportSize_.x, viewportSize_.y}, ImVec2{0, 1},
               ImVec2{1, 0});

  if (!isGameView_) {
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
        if (ImGui::Button(label, ImVec2(28, 22)))
          gizmoMode_ = mode;
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

      ImGui::Checkbox("Grid", &showGrid_);
      ImGui::SameLine();
      ImGui::Checkbox("Snap", &snap_);
      if (snap_) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(50);
        ImGui::DragFloat("##SnapValue", &snapValue_, 0.05f, 0.05f, 5.0f, "%.2f");
      }
      ImGui::SameLine(ImGui::GetWindowWidth() - 80.0f);
      ImGui::SetNextItemWidth(60);
      ImGui::ColorEdit4("##Clear", &clearColor_.x,
                        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
    
      if (showGrid_ && viewportSize_.x > 0 && viewportSize_.y > 0) {
        float aspectRatio = viewportSize_.x / viewportSize_.y;
        renderer::Renderer2D::BeginScene(
            renderer::OrthographicCamera(-aspectRatio, aspectRatio, -1.0f, 1.0f));
    
        float gridSpacing = 1.0f;
        Math::Vec4f gridColor = {0.3f, 0.3f, 0.3f, 1.0f};
    
        for (float x = -10.0f; x <= 10.0f; x += gridSpacing) {
          renderer::Renderer2D::DrawQuad({x, 0.0f, -0.1f}, {0.02f, 20.0f},
                                         gridColor);
        }
        for (float y = -10.0f; y <= 10.0f; y += gridSpacing) {
          renderer::Renderer2D::DrawQuad({0.0f, y, -0.1f}, {20.0f, 0.02f},
                                         gridColor);
        }
        renderer::Renderer2D::EndScene();
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
            EditorToolbar::GetHierarchyPanel()->SetSelectedEntity(
                ecs::INVALID_ENTITY);
          } else {
            // Convert pixel index to Entity handle
            // Since handles are index-based, we can reconstruct it (assuming
            // version 0 for now or finding a way to get the real entity) Better:
            // The pixelData IS the index. We need to find the entity with this
            // index. For now, let's create a temp entity to test selection
            EditorToolbar::GetHierarchyPanel()->SetSelectedEntity(
                ecs::Entity::Create((uint32_t)pixelData, 0));
          }
        }
      }
    
      // Gizmos
      ecs::Entity selectedEntity =
          EditorToolbar::GetHierarchyPanel()->GetSelectedEntity();
      if (selectedEntity && sceneContext_ &&
          sceneContext_->HasComponent<ecs::TransformComponent>(selectedEntity)) {
        ImGuizmo::SetOrthographic(true);
        ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
        ImGuizmo::SetRect(viewportBounds_[0].x, viewportBounds_[0].y,
                          viewportBounds_[1].x - viewportBounds_[0].x,
                          viewportBounds_[1].y - viewportBounds_[0].y);
    
        // Setup camera matrices for 2D orthographic projection
        float view[16];
        float projection[16];
    
        // View matrix with camera offset
        for (int i = 0; i < 16; i++) {
          view[i] = (i % 5 == 0) ? 1.0f : 0.0f;
          projection[i] = (i % 5 == 0) ? 1.0f : 0.0f;
        }
        view[12] = -cameraPosition_.x;
        view[13] = -cameraPosition_.y;
    
        // Calculate aspect-ratio aware orthographic projection
        if (viewportSizeNorm.y > 0) {
          projection[0] =
              2.0f / viewportSizeNorm.x * (viewportSizeNorm.x / viewportSizeNorm.y);
          projection[5] = 2.0f / viewportSizeNorm.y;
        }
        projection[10] = 1.0f;
    
        auto &tc =
            sceneContext_->GetComponent<ecs::TransformComponent>(selectedEntity);
        Math::Mat4f transform = Math::Mat4f::Translate(tc.position) *
                                tc.rotation.ToMat4x4() *
                                Math::Mat4f::Scale(tc.scale);
    
        float snapValues[3] = {snapValue_, snapValue_, snapValue_};

        ImGuizmo::OPERATION op = ImGuizmo::TRANSLATE;
        if (gizmoMode_ == 1) op = ImGuizmo::ROTATE;
        else if (gizmoMode_ == 2) op = ImGuizmo::SCALE;

        ImGuizmo::Manipulate(view, projection, op, ImGuizmo::LOCAL,
                             transform.Data(), nullptr,
                             snap_ ? snapValues : nullptr);
    
        if (ImGuizmo::IsUsing()) {
          if (!isUsingGizmo_) {
            // Started using gizmo - capture starting state
            startingPosition_ = tc.position;
            startingRotation_ = tc.rotation;
            startingScale_ = tc.scale;
            isUsingGizmo_ = true;
          }

          // Extract position from column 3
          tc.position = {transform.cols[3].x, transform.cols[3].y,
                         transform.cols[3].z};
          // For scale: extract column lengths
          if (gizmoMode_ == 2) {
            auto colLen = [](const Math::Vec4f& c) {
              return sqrtf(c.x * c.x + c.y * c.y + c.z * c.z);
            };
            tc.scale.x = colLen(transform.cols[0]);
            tc.scale.y = colLen(transform.cols[1]);
            tc.scale.z = colLen(transform.cols[2]);
          }
          // Note: Extraction for rotation is more complex and would involve
          // DecomposeMatrix; skipping for now as rotations are locked to Z in 2D
          // but if implemented, it would go here.
        } else if (isUsingGizmo_) {
          // Just stopped using gizmo - push final command
          cmd::CommandHistory::PushCommand(std::make_unique<cmd::CommandChangeTransform>(
              *sceneContext_, selectedEntity, 
              startingPosition_, tc.position,
              startingRotation_, tc.rotation,
              startingScale_, tc.scale
          ));
          isUsingGizmo_ = false;
        }
      }
    
      // Render viewport-space indicators for specialized entities (SpawnPoints,
      // etc.)
      if (sceneContext_) {
        auto drawList = ImGui::GetWindowDrawList();
        // Iterate through all entities that have a TagComponent
        for (auto e : sceneContext_->Query<ecs::TagComponent>()) {
          if (sceneContext_->HasComponent<ecs::TransformComponent>(e)) {
            auto &tag = sceneContext_->GetComponent<ecs::TagComponent>(e).tag;
            if (tag == "SpawnPoint") {
              auto &tc = sceneContext_->GetComponent<ecs::TransformComponent>(e);
              // Project 3D/2D world pos to screen pos
              // For now, assuming world == screen with offset
              ImVec2 pos = {viewportBounds_[0].x +
                                (tc.position.x + 1.0f) * 0.5f * viewportSize_.x,
                            viewportBounds_[0].y +
                                (1.0f - (tc.position.y + 1.0f) * 0.5f) *
                                    viewportSize_.y};
    
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
