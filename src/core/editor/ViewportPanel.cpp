#include "ViewportPanel.h"
#include "../ecs/World.h"
#include "../ecs/components/TagComponent.h"
#include "../ecs/components/TransformComponent.h"
#include "../renderer/Renderer2D.h"
#include "EditorToolbar.h"
#include <ImGuizmo.h>

namespace ge {
namespace editor {

ViewportPanel::ViewportPanel() {
  renderer::FramebufferSpecification spec;
  spec.Width = 1280;
  spec.Height = 720;
  framebuffer_ = renderer::Framebuffer::Create(spec);
}

void ViewportPanel::OnImGuiRender() {
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
  ImGui::Begin("Viewport");

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
  if (isHovered_ && ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
    ImVec2 delta = ImGui::GetIO().MouseDelta;
    cameraPosition_.x -= delta.x * (2.0f / viewportSize_.x);
    cameraPosition_.y += delta.y * (2.0f / viewportSize_.y);
  }

  uint32_t textureID = framebuffer_->GetColorAttachmentRendererID();
  ImGui::Image((void *)(uintptr_t)textureID,
               ImVec2{viewportSize_.x, viewportSize_.y}, ImVec2{0, 1},
               ImVec2{1, 0});

  // Grid UI Toggle Overlay
  ImGui::SetCursorPos(ImVec2(10, 30));
  ImGui::Checkbox("Show Grid", &showGrid_);
  ImGui::SameLine();
  ImGui::Checkbox("Snap", &snap_);
  if (snap_) {
    ImGui::SameLine();
    ImGui::SetNextItemWidth(50);
    ImGui::DragFloat("##SnapValue", &snapValue_, 0.05f, 0.05f, 5.0f, "%.2f");
  }

  if (showGrid_) {
    renderer::Renderer2D::BeginScene(
        OrthographicCamera(-viewportSize.x / viewportSize.y,
                           viewportSize.x / viewportSize.y, -1.0f,
                           1.0f)); // Simple camera for grid

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
    projection[0] =
        2.0f / viewportSizeNorm.x * (viewportSizeNorm.x / viewportSizeNorm.y);
    projection[5] = 2.0f / viewportSizeNorm.y;
    projection[10] = 1.0f;

    auto &tc =
        sceneContext_->GetComponent<ecs::TransformComponent>(selectedEntity);
    Math::Mat4f transform = Math::Mat4f::Translate(tc.position) *
                            tc.rotation.ToMat4x4() *
                            Math::Mat4f::Scale(tc.scale);

    float snapValues[3] = {snapValue_, snapValue_, snapValue_};
    ImGuizmo::Manipulate(view, projection, ImGuizmo::TRANSLATE, ImGuizmo::LOCAL,
                         transform.Data(), nullptr,
                         snap_ ? snapValues : nullptr);

    if (ImGuizmo::IsUsing()) {
      // Update entity position from gizmo transform
      tc.position = {transform.cols[3].x, transform.cols[3].y,
                     transform.cols[3].z};
    }
  }

  // Render viewport-space indicators for specialized entities (SpawnPoints,
  // etc.)
  if (sceneContext_) {
    auto drawList = ImGui::GetWindowDrawList();
    // Iterate through entities with TagComponent (this is inefficient, but okay
    // for a few indicators)
    for (uint32_t i = 0; i < 10000; i++) {
      ecs::Entity e(i);
      if (sceneContext_->IsAlive(e) &&
          sceneContext_->HasComponent<ecs::TagComponent>(e) &&
          sceneContext_->HasComponent<ecs::TransformComponent>(e)) {
        auto &tag = sceneContext_->GetComponent<ecs::TagComponent>(e).tag;
        if (tag == "SpawnPoint") {
          auto &tc = sceneContext_->GetComponent<ecs::TransformComponent>(e);
          // Project 3D/2D world pos to screen pos
          // For now, assuming world == screen with offset
          ImVec2 pos = {viewportBounds_[0].x +
                            (tc.position.x + 1.0f) * 0.5f * viewportSize.x,
                        viewportBounds_[0].y +
                            (1.0f - (tc.position.y + 1.0f) * 0.5f) *
                                viewportSize.y};

          drawList->AddCircleFilled(pos, 10.0f, IM_COL32(0, 255, 0, 200));
          drawList->AddText(ImVec2(pos.x + 12, pos.y - 6),
                            IM_COL32(255, 255, 255, 255), "Spawn Point");
        }
      }
    }
  }

  ImGui::End();
  ImGui::PopStyleVar();
}

} // namespace editor
} // namespace ge
