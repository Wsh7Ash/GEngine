#include "SceneHierarchyPanel.h"
#include "../ecs/ScriptableEntity.h"
#include "../ecs/components/MeshComponent.h"
#include "../ecs/components/LightComponent.h"
#include "../ecs/components/NativeScriptComponent.h"
#include "../ecs/components/SpriteComponent.h"
#include "../ecs/components/TagComponent.h"
#include "../ecs/components/TransformComponent.h"
#include "../ecs/components/Rigidbody2DComponent.h"
#include "../ecs/components/BoxCollider2DComponent.h"
#include "../ecs/components/RelationshipComponent.h"
#include "../renderer/Renderer2D.h"
#include "../cmd/CommandHistory.h"
#include "../cmd/EntityCommands.h"
#include "../math/MathUtils.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <nlohmann/json.hpp>
#include "../ecs/ScriptRegistry.h"
#include "../scene/PrefabSerializer.h"

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

  // Iterate through all entities that have a TagComponent
  std::string filter(search_filter_);
  std::transform(filter.begin(), filter.end(), filter.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  for (auto entity : context_->Query<ecs::TagComponent>()) {
    if (!filter.empty()) {
        DrawEntityNode(entity);
    } else {
        // Only start drawing from root entities (no parent) when not filtering
        bool hasRelationship = context_->HasComponent<ecs::RelationshipComponent>(entity);
        bool hasParent = false;
        if (hasRelationship) {
            hasParent = context_->GetComponent<ecs::RelationshipComponent>(entity).Parent != ecs::INVALID_ENTITY;
        }

        if (!hasParent) {
            DrawEntityNode(entity);
        }
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

  // --- Drag and Drop ---
  if (ImGui::BeginDragDropSource()) {
      ImGui::SetDragDropPayload("ENTITY_REORDER", &entity, sizeof(ecs::Entity));
      ImGui::Text("%s", tag.c_str());
      ImGui::EndDragDropSource();
  }

  if (ImGui::BeginDragDropTarget()) {
      if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_REORDER")) {
          ecs::Entity droppedEntity = *(const ecs::Entity*)payload->Data;
          context_->SetParent(droppedEntity, entity);
      }
      ImGui::EndDragDropTarget();
  }

  bool entityDeleted = false;
  if (ImGui::BeginPopupContextItem()) {
    if (ImGui::MenuItem("Delete Entity"))
      entityDeleted = true;

    if (ImGui::MenuItem("Save as Prefab...")) {
        std::string prefabTag = context_->HasComponent<ecs::TagComponent>(entity) 
            ? context_->GetComponent<ecs::TagComponent>(entity).tag 
            : "Entity";
        std::string filepath = "assets/" + prefabTag + ".prefab";
        scene::PrefabSerializer::Serialize(*context_, entity, filepath);
    }

    ImGui::EndPopup();
  }

  if (opened) {
    if (context_->HasComponent<ecs::RelationshipComponent>(entity)) {
        auto& rc = context_->GetComponent<ecs::RelationshipComponent>(entity);
        for (auto child : rc.Children) {
            DrawEntityNode(child);
        }
    }
    ImGui::TreePop();
  }

  if (entityDeleted) {
    context_->DestroyEntity(entity);
    if (selection_context_ == entity)
      selection_context_ = ecs::Entity();
  }
}

static bool DrawVec3Control(const std::string &label, Math::Vec3f &values,
                            float resetValue = 0.0f,
                            float columnWidth = 100.0f) {
  bool changed = false;
  ImGuiIO &io = ImGui::GetIO();
  auto boldFont = io.Fonts->Fonts[0]; // Assuming 0 is default/bold enough

  ImGui::PushID(label.c_str());

  ImGui::TableNextRow();
  ImGui::TableNextColumn();
  ImGui::Text(label.c_str());
  ImGui::TableNextColumn();

  ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

  float lineHeight =
      ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
  ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};

  // X
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.9f, 0.2f, 0.2f, 1.00f});
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
  ImGui::PushFont(boldFont);
  if (ImGui::Button("X", buttonSize)) {
    values.x = resetValue;
    changed = true;
  }
  ImGui::PopFont();
  ImGui::PopStyleColor(3);

  ImGui::SameLine();
  ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
  if (ImGui::IsItemDeactivatedAfterEdit()) changed = true;
  ImGui::PopItemWidth();
  ImGui::SameLine();

  // Y
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.3f, 0.8f, 0.3f, 1.00f});
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
  ImGui::PushFont(boldFont);
  if (ImGui::Button("Y", buttonSize)) {
    values.y = resetValue;
    changed = true;
  }
  ImGui::PopFont();
  ImGui::PopStyleColor(3);

  ImGui::SameLine();
  ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
  if (ImGui::IsItemDeactivatedAfterEdit()) changed = true;
  ImGui::PopItemWidth();
  ImGui::SameLine();

  // Z
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.1f, 0.25f, 0.8f, 1.00f});
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.2f, 0.35f, 0.9f, 1.00f});
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.1f, 0.25f, 0.8f, 1.00f});
  ImGui::PushFont(boldFont);
  if (ImGui::Button("Z", buttonSize)) {
    values.z = resetValue;
    changed = true;
  }
  ImGui::PopFont();
  ImGui::PopStyleColor(3);

  ImGui::SameLine();
  ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
  if (ImGui::IsItemDeactivatedAfterEdit()) changed = true;
  ImGui::PopItemWidth();

  ImGui::PopStyleVar();
  ImGui::PopID();

  return changed;
}

void SceneHierarchyPanel::DrawComponents(ecs::Entity entity) {
  if (context_->HasComponent<ecs::TagComponent>(entity)) {
    auto &tag = context_->GetComponent<ecs::TagComponent>(entity).tag;

    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    strncpy(buffer, tag.c_str(), sizeof(buffer));
    if (ImGui::InputText("##Tag", buffer, sizeof(buffer))) {
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
    // Search bar
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##CompSearch", "Search components...",
                             component_search_, sizeof(component_search_));
    ImGui::Separator();

    std::string filter(component_search_);
    std::transform(filter.begin(), filter.end(), filter.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    auto showItem = [&](const char* label, auto addFn, bool enabled) {
      if (!filter.empty()) {
        std::string lower(label);
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        if (lower.find(filter) == std::string::npos) return;
      }
      if (ImGui::MenuItem(label, nullptr, false, enabled)) {
        addFn();
        component_search_[0] = '\0';
        ImGui::CloseCurrentPopup();
      }
    };

    // ── Rendering ──
    bool showRendering = filter.empty();
    if (!showRendering) {
      // Show header if any child matches
      for (auto name : {"Sprite Component", "Mesh Component", "Light Component"}) {
        std::string lower(name);
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        if (lower.find(filter) != std::string::npos) { showRendering = true; break; }
      }
    }
    if (showRendering) {
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.40f, 0.85f, 0.40f, 1.00f));
      ImGui::Text("Rendering");
      ImGui::PopStyleColor();
    }
    showItem("Sprite Component",
             [&]() { context_->AddComponent(entity, ecs::SpriteComponent{}); },
             !context_->HasComponent<ecs::SpriteComponent>(entity));
    showItem("Mesh Component",
             [&]() { context_->AddComponent(entity, ecs::MeshComponent{}); },
             !context_->HasComponent<ecs::MeshComponent>(entity));
    showItem("Light Component",
             [&]() { context_->AddComponent(entity, ecs::LightComponent{}); },
             !context_->HasComponent<ecs::LightComponent>(entity));

    // ── Scripting ──
    bool showScripting = filter.empty();
    if (!showScripting) {
      std::string lower("Native Script Component");
      std::transform(lower.begin(), lower.end(), lower.begin(),
                     [](unsigned char c) { return std::tolower(c); });
      if (lower.find(filter) != std::string::npos) showScripting = true;
    }
    if (showScripting) {
      ImGui::Separator();
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.65f, 0.13f, 1.00f));
      ImGui::Text("Scripting");
      ImGui::PopStyleColor();
    }
    showItem("Native Script Component",
             [&]() { context_->AddComponent(entity, ecs::NativeScriptComponent{}); },
             !context_->HasComponent<ecs::NativeScriptComponent>(entity));

    // ── Core ──
    bool showCore = filter.empty();
    if (!showCore) {
      std::string lower("Tag Component");
      std::transform(lower.begin(), lower.end(), lower.begin(),
                     [](unsigned char c) { return std::tolower(c); });
      if (lower.find(filter) != std::string::npos) showCore = true;
    }
    if (showCore) {
      ImGui::Separator();
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.00f, 0.71f, 0.85f, 1.00f));
      ImGui::Text("Core");
      ImGui::PopStyleColor();
    }
    showItem("Tag Component",
             [&]() { context_->AddComponent(entity, ecs::TagComponent{"New Entity"}); },
             !context_->HasComponent<ecs::TagComponent>(entity));

    showItem("Rigidbody 2D",
             [&]() { context_->AddComponent(entity, ecs::Rigidbody2DComponent{}); },
             !context_->HasComponent<ecs::Rigidbody2DComponent>(entity));

    showItem("Box Collider 2D",
             [&]() { context_->AddComponent(entity, ecs::BoxCollider2DComponent{}); },
             !context_->HasComponent<ecs::BoxCollider2DComponent>(entity));

    ImGui::EndPopup();
  }
  ImGui::PopItemWidth();
  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  if (context_->HasComponent<ecs::TransformComponent>(entity)) {
    DrawComponentControl<ecs::TransformComponent>("Transform", entity, [&]() {
      auto &tc = context_->GetComponent<ecs::TransformComponent>(entity);
      
      // Capture state before edit
      Math::Vec3f oldPos = tc.position;
      Math::Quatf oldRot = tc.rotation;
      Math::Vec3f oldScale = tc.scale;
      bool changed = false;

      if (ImGui::BeginTable("TransformTable", 2,
                            ImGuiTableFlags_Resizable |
                                ImGuiTableFlags_BordersInnerV)) {
        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed,
                                80.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        if (DrawVec3Control("Position", tc.position)) changed = true;

        Math::Vec3f euler = tc.rotation.ToEuler();
        Math::Vec3f degrees = { Math::RadiansToDegrees(euler.x), Math::RadiansToDegrees(euler.y), Math::RadiansToDegrees(euler.z) };
        if (DrawVec3Control("Rotation", degrees)) {
            tc.rotation = Math::Quatf::FromEuler(Math::DegreesToRadians(degrees.x), Math::DegreesToRadians(degrees.y), Math::DegreesToRadians(degrees.z));
            changed = true;
        }

        if (DrawVec3Control("Scale", tc.scale, 1.0f)) changed = true;

        ImGui::EndTable();
      }

      if (changed) {
        cmd::CommandHistory::PushCommand(std::make_unique<cmd::CommandChangeTransform>(
          *context_, entity, 
          oldPos, tc.position,
          oldRot, tc.rotation,
          oldScale, tc.scale
        ));
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

  if (context_->HasComponent<ecs::MeshComponent>(entity)) {
    DrawComponentControl<ecs::MeshComponent>("Mesh & Material (PBR)", entity, [&]() {
      auto &mc = context_->GetComponent<ecs::MeshComponent>(entity);
      
      if (ImGui::BeginTable("MeshMaterialTable", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV)) {
        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        // Mesh Path
        ImGui::TableNextRow();
        ImGui::TableNextColumn(); ImGui::Text("Mesh");
        ImGui::TableNextColumn(); ImGui::PushItemWidth(-1);
        ImGui::InputText("##meshpath", (char*)mc.MeshPath.c_str(), mc.MeshPath.capacity(), ImGuiInputTextFlags_ReadOnly);
        ImGui::PopItemWidth();

        // Albedo
        ImGui::TableNextRow();
        ImGui::TableNextColumn(); ImGui::Text("Albedo Color");
        ImGui::TableNextColumn(); ImGui::PushItemWidth(-1);
        ImGui::ColorEdit3("##albedocolor", &mc.AlbedoColor.x);
        ImGui::PopItemWidth();

        // Metallic
        ImGui::TableNextRow();
        ImGui::TableNextColumn(); ImGui::Text("Metallic");
        ImGui::TableNextColumn(); ImGui::PushItemWidth(-1);
        ImGui::SliderFloat("##metallic", &mc.Metallic, 0.0f, 1.0f);
        ImGui::PopItemWidth();

        // Roughness
        ImGui::TableNextRow();
        ImGui::TableNextColumn(); ImGui::Text("Roughness");
        ImGui::TableNextColumn(); ImGui::PushItemWidth(-1);
        ImGui::SliderFloat("##roughness", &mc.Roughness, 0.0f, 1.0f);
        ImGui::PopItemWidth();

        ImGui::Separator();

        // Textures
        auto DrawTextureSlot = [&](const char* label, std::string& path) {
          ImGui::TableNextRow();
          ImGui::TableNextColumn(); ImGui::Text("%s", label);
          ImGui::TableNextColumn(); ImGui::PushItemWidth(-1);
          std::string buttonLabel = path.empty() ? "None (Drop Texture)" : path;
          ImGui::Button(buttonLabel.c_str(), ImVec2(-1, 0));
          ImGui::PopItemWidth();
          // Drag & Drop logic placeholder
        };

        DrawTextureSlot("Albedo Map", mc.AlbedoPath);
        DrawTextureSlot("Normal Map", mc.NormalPath);
        DrawTextureSlot("Metallic Map", mc.MetallicPath);
        DrawTextureSlot("Roughness Map", mc.RoughnessPath);
        DrawTextureSlot("AO Map", mc.AOPath);

        ImGui::EndTable();
      }
    });
  }

  if (context_->HasComponent<ecs::LightComponent>(entity)) {
    DrawComponentControl<ecs::LightComponent>("Light", entity, [&]() {
      auto &lc = context_->GetComponent<ecs::LightComponent>(entity);
      
      const char* lightTypes[] = { "Directional", "Point" };
      int typeIndex = (int)lc.Type;
      if (ImGui::Combo("Light Type", &typeIndex, lightTypes, 2)) {
        lc.Type = (ecs::LightType)typeIndex;
      }

      ImGui::ColorEdit3("Color", &lc.Color.x);
      ImGui::DragFloat("Intensity", &lc.Intensity, 0.1f, 0.0f, 100.0f);
      if (lc.Type == ecs::LightType::Point) {
        ImGui::DragFloat("Range", &lc.Range, 0.1f, 0.0f, 1000.0f);
      }
      ImGui::Checkbox("Cast Shadows", &lc.CastShadows);
    });
  }

  if (context_->HasComponent<ecs::NativeScriptComponent>(entity)) {
    DrawComponentControl<ecs::NativeScriptComponent>(
        "Native Script", entity, [&]() {
          auto &nsc =
              context_->GetComponent<ecs::NativeScriptComponent>(entity);
          ImGui::Text("Bound Script: %s",
                      nsc.ScriptName.empty() ? "None" : nsc.ScriptName.c_str());
          ImGui::Text("Instance: %s", nsc.instance ? "Running" : "Idle");

          if (ImGui::BeginCombo("##ScriptSelection", nsc.ScriptName.c_str())) {
            for (const auto& scriptName : ecs::ScriptRegistry::GetAllNames()) {
              bool isSelected = (nsc.ScriptName == scriptName);
              if (ImGui::Selectable(scriptName.c_str(), isSelected)) {
                ecs::ScriptRegistry::BindByName(&nsc, scriptName);
              }
            }
            ImGui::EndCombo();
          }

          if (ImGui::Button("Remove Script")) {
            context_->RemoveComponent<ecs::NativeScriptComponent>(entity);
          }
        });
  }

  if (context_->HasComponent<ecs::Rigidbody2DComponent>(entity)) {
    DrawComponentControl<ecs::Rigidbody2DComponent>("Rigidbody 2D", entity, [&]() {
      auto &rb = context_->GetComponent<ecs::Rigidbody2DComponent>(entity);
      
      const char* bodyTypes[] = { "Static", "Dynamic", "Kinematic" };
      int bodyTypeIndex = (int)rb.Type;
      if (ImGui::Combo("Body Type", &bodyTypeIndex, bodyTypes, 3)) {
        rb.Type = (ecs::RigidBody2DType)bodyTypeIndex;
      }

      ImGui::Checkbox("Fixed Rotation", &rb.FixedRotation);
    });
  }

  if (context_->HasComponent<ecs::BoxCollider2DComponent>(entity)) {
    DrawComponentControl<ecs::BoxCollider2DComponent>("Box Collider 2D", entity, [&]() {
      auto &bc = context_->GetComponent<ecs::BoxCollider2DComponent>(entity);
      
      ImGui::DragFloat2("Offset", &bc.Offset.x, 0.1f);
      ImGui::DragFloat2("Size", &bc.Size.x, 0.1f);
      ImGui::DragFloat("Density", &bc.Density, 0.1f, 0.0f, 10.0f);
      ImGui::DragFloat("Friction", &bc.Friction, 0.1f, 0.0f, 1.0f);
      ImGui::DragFloat("Restitution", &bc.Restitution, 0.1f, 0.0f, 1.0f);
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
