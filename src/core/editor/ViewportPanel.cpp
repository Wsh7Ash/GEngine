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

  ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
  if (viewportSize_.x != viewportPanelSize.x ||
      viewportSize_.y != viewportPanelSize.y) {
    viewportSize_ = Math::Vec2f(viewportPanelSize.x, viewportPanelSize.y);
    framebuffer_->Resize((uint32_t)viewportSize_.x, (uint32_t)viewportSize_.y);
  }

  uint32_t textureID = framebuffer_->GetColorAttachmentRendererID();
  ImGui::Image((void *)(uintptr_t)textureID,
               ImVec2{viewportSize_.x, viewportSize_.y}, ImVec2{0, 1},
               ImVec2{1, 0});

  // Mouse Picking
  auto [mx, my] = ImGui::GetMousePos();
  mx -= viewportBounds_[0].x;
  my -= viewportBounds_[0].y;
  Math::Vec2f viewportSize = viewportBounds_[1] - viewportBounds_[0];
  my = viewportSize.y - my; // Invert Y for OpenGL

  int mouseX = (int)mx;
  int mouseY = (int)my;

  if (mouseX >= 0 && mouseY >= 0 && mouseX < (int)viewportSize.x &&
      mouseY < (int)viewportSize.y) {
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

    // Camera matrices (Placeholder for now, should come from active camera)
    float view[16];
    float projection[16];
    // Identity view for 2D
    for (int i = 0; i < 16; i++) {
      view[i] = (i % 5 == 0) ? 1.0f : 0.0f;
      projection[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    }

    // Simple 2D projection-like scale
    projection[0] = 2.0f / viewportSize.x * (viewportSize.x / viewportSize.y);
    projection[5] = 2.0f / viewportSize.y;
    projection[10] = 1.0f;

    auto &tc =
        sceneContext_->GetComponent<ecs::TransformComponent>(selectedEntity);
    Math::Mat4f transform = Math::Mat4f::Translate(tc.position) *
                            tc.rotation.ToMat4x4() *
                            Math::Mat4f::Scale(tc.scale);

    ImGuizmo::Manipulate(view, projection, ImGuizmo::TRANSLATE, ImGuizmo::LOCAL,
                         transform.Data());

    if (ImGuizmo::IsUsing()) {
      // Decompose and update tc
      // For now, just translation since decomposing is complex without a helper
      tc.position = {transform.cols[3].x, transform.cols[3].y,
                     transform.cols[3].z};
    }
  }

  // Draw indicators for specialized entities (e.g. SpawnPoints)
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
