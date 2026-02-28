#include "SceneHierarchyPanel.h"
#include "../ecs/components/NativeScriptComponent.h"
#include "../ecs/components/SpriteComponent.h"
#include "../ecs/components/TagComponent.h"
#include "../ecs/components/TransformComponent.h"
#include "../renderer/Renderer2D.h"
#include <imgui.h>

namespace ge {
namespace editor {

SceneHierarchyPanel::SceneHierarchyPanel() = default;

SceneHierarchyPanel::SceneHierarchyPanel(ecs::World &world)
    : context_(&world) {}

void SceneHierarchyPanel::SetContext(ecs::World &world) {
  context_ = &world;
  selection_context_ = ecs::Entity(); // Reset selection
}

void SceneHierarchyPanel::OnImGuiRender() {
  if (!context_)
    return;

  // 1. Hierarchy Window
  ImGui::Begin("Scene Hierarchy");

  // Iterate through all entities (using a fixed limit for now, but checking
  // IsAlive)
  for (uint32_t i = 0; i < 10000; ++i) {
    ecs::Entity entity(i);
    if (context_->IsAlive(entity)) {
      DrawEntityNode(entity);
    }
  }

  // Right-click on blank space
  if (ImGui::BeginPopupContextWindow()) {
    if (ImGui::MenuItem("Create Empty Entity")) {
      auto e = context_->CreateEntity();
      context_->AddComponent(e, ecs::TransformComponent{});
      context_->AddComponent(e, ecs::TagComponent{"Entity"});
    }
    ImGui::EndPopup();
  }

  if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
    selection_context_ = ecs::Entity();

  ImGui::End();

  // 2. Inspector Window
  ImGui::Begin("Inspector");
  if (selection_context_) {
    DrawComponents(selection_context_);
  }

  ImGui::End();
}

void SceneHierarchyPanel::DrawEntityNode(ecs::Entity entity) {
  std::string tag = context_->HasComponent<ecs::TagComponent>(entity)
                        ? context_->GetComponent<ecs::TagComponent>(entity).tag
                        : "Entity " + std::to_string(entity.GetIndex());

  ImGuiTreeNodeFlags flags =
      ((selection_context_ == entity) ? ImGuiTreeNodeFlags_Selected : 0) |
      ImGuiTreeNodeFlags_OpenOnArrow;
  flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
  bool opened = ImGui::TreeNodeEx((void *)(uint64_t)entity.GetIndex(), flags,
                                  tag.c_str());

  if (ImGui::IsItemClicked()) {
    selection_context_ = entity;
  }

  bool entityDeleted = false;
  if (ImGui::BeginPopupContextItem()) {
    if (ImGui::MenuItem("Delete Entity"))
      entityDeleted = true;

    ImGui::EndPopup();
  }

  if (opened) {
    // Placeholder for children
    ImGui::TreePop();
  }

  if (entityDeleted) {
    context_->DestroyEntity(entity);
    if (selection_context_ == entity)
      selection_context_ = ecs::Entity();
  }
}

void SceneHierarchyPanel::DrawComponents(ecs::Entity entity) {
  if (context_->HasComponent<ecs::TagComponent>(entity)) {
    auto &tag = context_->GetComponent<ecs::TagComponent>(entity).tag;
    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "%s", tag.c_str());
    if (ImGui::InputText("Tag", buffer, sizeof(buffer))) {
      tag = std::string(buffer);
    }
  }

  ImGui::Spacing();
  ImGui::SameLine();
  ImGui::PushItemWidth(-1);

  // Accent-styled Add Component button
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.00f, 0.45f, 0.55f, 0.80f));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                        ImVec4(0.00f, 0.60f, 0.72f, 1.00f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                        ImVec4(0.00f, 0.35f, 0.45f, 1.00f));
  if (ImGui::Button("+ Add Component", ImVec2(-1, 0)))
    ImGui::OpenPopup("AddComponent");
  ImGui::PopStyleColor(3);

  if (ImGui::BeginPopup("AddComponent")) {
    if (ImGui::MenuItem("Sprite Component")) {
      if (!context_->HasComponent<ecs::SpriteComponent>(entity))
        context_->AddComponent(entity, ecs::SpriteComponent{});
      ImGui::CloseCurrentPopup();
    }

    if (ImGui::MenuItem("Native Script Component")) {
      if (!context_->HasComponent<ecs::NativeScriptComponent>(entity))
        context_->AddComponent(entity, ecs::NativeScriptComponent{});
      ImGui::CloseCurrentPopup();
    }

    if (ImGui::MenuItem("Tag Component")) {
      if (!context_->HasComponent<ecs::TagComponent>(entity))
        context_->AddComponent(entity, ecs::TagComponent{"New Entity"});
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }
  ImGui::PopItemWidth();
  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  if (context_->HasComponent<ecs::TransformComponent>(entity)) {
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
      auto &tc = context_->GetComponent<ecs::TransformComponent>(entity);
      if (ImGui::BeginTable("TransformTable", 2,
                            ImGuiTableFlags_Resizable |
                                ImGuiTableFlags_BordersInnerV)) {
        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed,
                                80.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Position");
        ImGui::TableNextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::DragFloat3("##pos", &tc.position.x, 0.1f);
        ImGui::PopItemWidth();

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Rotation");
        ImGui::TableNextColumn();
        static Math::Vec3f rotation = {0, 0, 0};
        ImGui::PushItemWidth(-1);
        if (ImGui::DragFloat3("##rot", &rotation.x, 0.1f)) {
          tc.rotation = Math::Quatf::FromEuler(rotation);
        }
        ImGui::PopItemWidth();

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Scale");
        ImGui::TableNextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::DragFloat3("##scale", &tc.scale.x, 0.1f);
        ImGui::PopItemWidth();

        ImGui::EndTable();
      }
    }
  }

  if (context_->HasComponent<ecs::SpriteComponent>(entity)) {
    auto &sc = context_->GetComponent<ecs::SpriteComponent>(entity);
    if (ImGui::CollapsingHeader("Sprite", ImGuiTreeNodeFlags_DefaultOpen)) {
      if (ImGui::BeginTable("SpriteTable", 2,
                            ImGuiTableFlags_Resizable |
                                ImGuiTableFlags_BordersInnerV)) {
        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed,
                                80.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Color");
        ImGui::TableNextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::ColorEdit4("##color", &sc.color.x);
        ImGui::PopItemWidth();

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Texture");
        ImGui::TableNextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::Button("Texture Slot", ImVec2(-1, 0));
        ImGui::PopItemWidth();
        if (ImGui::BeginDragDropTarget()) {
          if (const ImGuiPayload *payload =
                  ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM")) {
            // Path-probing and loading logic would go here
            GE_LOG_INFO("Texture drag-and-drop received.");
          }
          ImGui::EndDragDropTarget();
        }

        ImGui::EndTable();
      }
    }
  }

  if (context_->HasComponent<ecs::NativeScriptComponent>(entity)) {
    if (ImGui::CollapsingHeader("Native Script",
                                ImGuiTreeNodeFlags_DefaultOpen)) {
      auto &nsc = context_->GetComponent<ecs::NativeScriptComponent>(entity);
      ImGui::Text("Instance: %s", nsc.instance ? "Active" : "None");
      if (ImGui::Button("Remove Script")) {
        context_->RemoveComponent<ecs::NativeScriptComponent>(entity);
      }
    }
  }
}

} // namespace editor
} // namespace ge
