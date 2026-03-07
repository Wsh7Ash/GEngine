#include "SceneHierarchyPanel.h"
#include "../ecs/components/NativeScriptComponent.h"
#include "../ecs/components/SpriteComponent.h"
#include "../ecs/components/TagComponent.h"
#include "../ecs/components/TransformComponent.h"
#include "../renderer/Renderer2D.h"
#include <imgui.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

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

  ImGui::InputText("##Search", search_filter_, sizeof(search_filter_));
  ImGui::SameLine();
  if (ImGui::Button("Clear")) {
    search_filter_[0] = '\0';
  }
  ImGui::Separator();

  // Iterate through all entities (using a fixed limit for now, but checking
  // IsAlive)
  std::string filter(search_filter_);
  std::transform(filter.begin(), filter.end(), filter.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  for (uint32_t i = 0; i < 10000; ++i) {
    ecs::Entity entity(i);
    if (context_->IsAlive(entity)) {
      if (!filter.empty()) {
        std::string tag =
            context_->HasComponent<ecs::TagComponent>(entity)
                ? context_->GetComponent<ecs::TagComponent>(entity).tag
                : "Entity " + std::to_string(entity.GetIndex());
        std::transform(tag.begin(), tag.end(), tag.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        if (tag.find(filter) == std::string::npos)
          continue;
      }
      DrawEntityNode(entity);
    }
  }

  // Right-click on blank space
  if (ImGui::BeginPopupContextWindow()) {
    if (ImGui::BeginMenu("Create")) {
      if (ImGui::MenuItem("Empty Entity")) {
        auto e = context_->CreateEntity();
        context_->AddComponent(e, ecs::TransformComponent{});
        context_->AddComponent(e, ecs::TagComponent{"Entity"});
      }

      if (ImGui::MenuItem("Sprite")) {
        auto e = context_->CreateEntity();
        context_->AddComponent(e, ecs::TransformComponent{});
        context_->AddComponent(e, ecs::TagComponent{"Sprite"});
        context_->AddComponent(e, ecs::SpriteComponent{});
      }

      if (ImGui::MenuItem("Spawn Point")) {
        auto e = context_->CreateEntity();
        context_->AddComponent(e, ecs::TransformComponent{});
        context_->AddComponent(e, ecs::TagComponent{"SpawnPoint"});
      }

      ImGui::EndMenu();
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
    if (ImGui::MenuItem(
            "Sprite Component", nullptr, false,
            !context_->HasComponent<ecs::SpriteComponent>(entity))) {
      context_->AddComponent(entity, ecs::SpriteComponent{});
      ImGui::CloseCurrentPopup();
    }

    if (ImGui::MenuItem(
            "Native Script Component", nullptr, false,
            !context_->HasComponent<ecs::NativeScriptComponent>(entity))) {
      context_->AddComponent(entity, ecs::NativeScriptComponent{});
      ImGui::CloseCurrentPopup();
    }

    if (ImGui::MenuItem("Tag Component", nullptr, false,
                        !context_->HasComponent<ecs::TagComponent>(entity))) {
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
    DrawComponentControl<ecs::TransformComponent>("Transform", entity, [&]() {
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
    });
  }

  if (context_->HasComponent<ecs::SpriteComponent>(entity)) {
    DrawComponentControl<ecs::SpriteComponent>("Sprite", entity, [&]() {
      auto &sc = context_->GetComponent<ecs::SpriteComponent>(entity);
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
            GE_LOG_INFO("Texture drag-and-drop received.");
          }
          ImGui::EndDragDropTarget();
        }

        ImGui::EndTable();
      }
    });
  }

  if (context_->HasComponent<ecs::NativeScriptComponent>(entity)) {
    DrawComponentControl<ecs::NativeScriptComponent>(
        "Native Script", entity, [&]() {
          auto &nsc =
              context_->GetComponent<ecs::NativeScriptComponent>(entity);
          ImGui::Text("Instance: %s", nsc.instance ? "Active" : "None");
          if (ImGui::Button("Remove Script")) {
            context_->RemoveComponent<ecs::NativeScriptComponent>(entity);
          }
        });
  }
}

template <typename T, typename UIFunction>
void SceneHierarchyPanel::DrawComponentControl(const std::string &name,
                                               ecs::Entity entity,
                                               UIFunction uiFunction) {
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
  float lineHeight =
      ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
  ImGui::Separator();
  bool open = ImGui::CollapsingHeader(name.c_str(),
                                      ImGuiTreeNodeFlags_DefaultOpen |
                                          ImGuiTreeNodeFlags_AllowOverlap);
  ImGui::PopStyleVar();

  ImGui::SameLine(ImGui::GetWindowWidth() - 25.0f);
  if (ImGui::Button("...", ImVec2(20.0f, lineHeight))) {
    ImGui::OpenPopup("ComponentSettings");
  }

  bool removeComponent = false;
  if (ImGui::BeginPopup("ComponentSettings")) {
    if (ImGui::MenuItem("Copy Component")) {
      json j;
      if (std::is_same<T, ecs::TransformComponent>::value) {
        auto &tc = context_->GetComponent<ecs::TransformComponent>(entity);
        j["Translation"] = {tc.position.x, tc.position.y, tc.position.z};
        j["Rotation"] = {tc.rotation.w, tc.rotation.x, tc.rotation.y,
                         tc.rotation.z};
        j["Scale"] = {tc.scale.x, tc.scale.y, tc.scale.z};
      } else if (std::is_same<T, ecs::SpriteComponent>::value) {
        auto &sc = context_->GetComponent<ecs::SpriteComponent>(entity);
        j["Color"] = {sc.color.x, sc.color.y, sc.color.z, sc.color.w};
        j["FlipX"] = sc.FlipX;
        j["FlipY"] = sc.FlipY;
        j["isAnimated"] = sc.isAnimated;
        j["framesX"] = sc.framesX;
        j["framesY"] = sc.framesY;
        j["frameTime"] = sc.frameTime;
      }
      component_clipboard_ = j.dump();
    }

    if (ImGui::MenuItem("Paste Component", nullptr, false,
                        !component_clipboard_.empty())) {
      try {
        json j = json::parse(component_clipboard_);
        if (std::is_same<T, ecs::TransformComponent>::value) {
          auto &tc = context_->GetComponent<ecs::TransformComponent>(entity);
          if (j.contains("Translation"))
            tc.position = {(float)j["Translation"][0],
                           (float)j["Translation"][1],
                           (float)j["Translation"][2]};
          if (j.contains("Rotation"))
            tc.rotation = {(float)j["Rotation"][0], (float)j["Rotation"][1],
                           (float)j["Rotation"][2], (float)j["Rotation"][3]};
          if (j.contains("Scale"))
            tc.scale = {(float)j["Scale"][0], (float)j["Scale"][1],
                        (float)j["Scale"][2]};
        } else if (std::is_same<T, ecs::SpriteComponent>::value) {
          auto &sc = context_->GetComponent<ecs::SpriteComponent>(entity);
          if (j.contains("Color"))
            sc.color = {(float)j["Color"][0], (float)j["Color"][1],
                        (float)j["Color"][2], (float)j["Color"][3]};
          if (j.contains("FlipX"))
            sc.FlipX = j["FlipX"];
          if (j.contains("FlipY"))
            sc.FlipY = j["FlipY"];
          if (j.contains("isAnimated"))
            sc.isAnimated = j["isAnimated"];
          if (j.contains("framesX"))
            sc.framesX = j["framesX"];
          if (j.contains("framesY"))
            sc.framesY = j["framesY"];
          if (j.contains("frameTime"))
            sc.frameTime = j["frameTime"];
        }
      } catch (...) {
        GE_LOG_ERROR("Failed to paste component: Invalid clipboard data.");
      }
    }

    if (ImGui::MenuItem("Remove Component")) {
      removeComponent = true;
    }

    ImGui::EndPopup();
  }

  if (open) {
    uiFunction();
    ImGui::Spacing();
  }

  if (removeComponent) {
    context_->RemoveComponent<T>(entity);
  }
}
} // namespace editor
} // namespace ge
