#include "ViewportPanel.h"
#include <imgui.h>

namespace ge {
namespace editor {

    ViewportPanel::ViewportPanel()
    {
        renderer::FramebufferSpecification spec;
        spec.Width = 1280;
        spec.Height = 720;
        framebuffer_ = renderer::Framebuffer::Create(spec);
    }

    void ViewportPanel::OnImGuiRender()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
        ImGui::Begin("Viewport");

        isFocused_ = ImGui::IsWindowFocused();
        isHovered_ = ImGui::IsWindowHovered();

        ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
        if (viewportSize_.x != viewportPanelSize.x || viewportSize_.y != viewportPanelSize.y)
        {
            viewportSize_ = Math::Vec2f(viewportPanelSize.x, viewportPanelSize.y);
            framebuffer_->Resize((uint32_t)viewportSize_.x, (uint32_t)viewportSize_.y);
        }

        uint32_t textureID = framebuffer_->GetColorAttachmentRendererID();
        ImGui::Image((void*)(uintptr_t)textureID, ImVec2{ viewportSize_.x, viewportSize_.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

        ImGui::End();
        ImGui::PopStyleVar();
    }

} // namespace editor
} // namespace ge
