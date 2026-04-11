#include "SceneHierarchyPanel.h"
#include "EditorToolbar.h"
#include "EditorPaths.h"
#include "PrecisionEditTool.h"
#include "../ecs/ScriptableEntity.h"
#include "../ecs/components/MeshComponent.h"
#include "../ecs/components/LightComponent.h"
#include "../ecs/components/NativeScriptComponent.h"
#include "../ecs/components/SpriteComponent.h"
#include "../ecs/components/TagComponent.h"
#include "../ecs/components/TransformComponent.h"
#include "../ecs/components/Rigidbody2DComponent.h"
#include "../ecs/components/BoxCollider2DComponent.h"
#include "../ecs/components/Rigidbody3DComponent.h"
#include "../ecs/components/Collider3DComponent.h"
#include "../ecs/components/RelationshipComponent.h"
#include "../ecs/components/TilemapComponent.h"
#include "../renderer/Renderer2D.h"
#include "../renderer/Texture.h"
#include "../cmd/CommandHistory.h"
#include "../cmd/EntityCommands.h"
#include "../math/MathUtils.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <nlohmann/json.hpp>
#include "../ecs/ScriptRegistry.h"
#include "../scene/PrefabSerializer.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <filesystem>

using json = nlohmann::json;

namespace ge {
namespace editor {

namespace {

std::filesystem::path ResolveAssetPath(const std::string& storedPath) {
  if (storedPath.empty()) {
    return {};
  }

  std::filesystem::path path(storedPath);
  if (path.is_relative()) {
    const auto assetRoot = EditorToolbar::GetAssetRoot().empty()
                               ? EditorPaths::ResolveAssetRoot()
                               : EditorToolbar::GetAssetRoot();
    path = assetRoot / path;
  }

  return EditorPaths::NormalizePath(path);
}

std::string MakeAssetRelativePath(const std::filesystem::path& path) {
  if (path.empty()) {
    return {};
  }

  const auto normalizedPath = EditorPaths::NormalizePath(path);
  const auto assetRoot = EditorToolbar::GetAssetRoot().empty()
                             ? EditorPaths::ResolveAssetRoot()
                             : EditorToolbar::GetAssetRoot();

  if (!assetRoot.empty()) {
    std::error_code ec;
    const auto relativePath = std::filesystem::relative(normalizedPath, assetRoot, ec);
    const auto relativeString = relativePath.generic_string();
    if (!ec && !relativeString.empty() && relativeString.rfind("..", 0) != 0) {
      return relativePath.generic_string();
    }
  }

  return normalizedPath.generic_string();
}

void ResizeTilemapStorage(ecs::TilemapComponent& tilemap, int width, int height) {
  tilemap.Width = (std::max)(0, width);
  tilemap.Height = (std::max)(0, height);

  const size_t cellCount =
      static_cast<size_t>((std::max)(0, tilemap.Width) * (std::max)(0, tilemap.Height));

  for (auto& layer : tilemap.Layers) {
    layer.Tiles.resize(cellCount, -1);
  }

  tilemap.Navigation.Resize(tilemap.Width, tilemap.Height, false);
}

void EnsureTilemapStorage(ecs::TilemapComponent& tilemap) {
  if (tilemap.Layers.empty()) {
    ecs::TilemapLayer layer;
    layer.Name = "Ground";
    layer.Tiles.resize(static_cast<size_t>((std::max)(0, tilemap.Width) * (std::max)(0, tilemap.Height)), -1);
    tilemap.Layers.push_back(std::move(layer));
  }

  ResizeTilemapStorage(tilemap, tilemap.Width, tilemap.Height);
}

void RebuildTilemapNavigation(ecs::TilemapComponent& tilemap) {
  if (tilemap.Width <= 0 || tilemap.Height <= 0) {
    tilemap.Navigation.Resize(0, 0, false);
    return;
  }

  tilemap.Navigation.Resize(tilemap.Width, tilemap.Height, false);
  tilemap.Navigation.CellSize = tilemap.PixelsPerUnit > 0.0f
      ? static_cast<float>(tilemap.TileWidth) / tilemap.PixelsPerUnit
      : 1.0f;

  std::fill(tilemap.Navigation.Blocked.begin(), tilemap.Navigation.Blocked.end(), 0u);
  for (const auto& layer : tilemap.Layers) {
    if (!layer.CollisionLayer) {
      continue;
    }

    for (int y = 0; y < tilemap.Height; ++y) {
      for (int x = 0; x < tilemap.Width; ++x) {
        const int index = y * tilemap.Width + x;
        if (index >= 0 && index < static_cast<int>(layer.Tiles.size()) &&
            layer.Tiles[static_cast<size_t>(index)] >= 0) {
          tilemap.Navigation.Blocked[static_cast<size_t>(index)] = 1u;
        }
      }
    }
  }
}

std::shared_ptr<renderer::Texture> LoadPixelTexture(const std::string& texturePath) {
  if (texturePath.empty()) {
    return nullptr;
  }

  renderer::TextureSpecification specification;
  specification.PixelArt = true;
  return renderer::Texture::Create(ResolveAssetPath(texturePath).string(), specification);
}

bool PushTransformCommandIfChanged(ecs::World& world,
                                   ecs::Entity entity,
                                   const Math::Vec3f& oldPos,
                                   const Math::Vec3f& newPos,
                                   const Math::Quatf& oldRot,
                                   const Math::Quatf& newRot,
                                   const Math::Vec3f& oldScale,
                                   const Math::Vec3f& newScale) {
  const bool changed = !oldPos.ApproxEqual(newPos, 0.0001f) ||
                       !oldRot.RotationEqual(newRot, 0.0001f) ||
                       !oldScale.ApproxEqual(newScale, 0.0001f);
  if (!changed) {
    return false;
  }

  cmd::CommandHistory::PushCommand(std::make_unique<cmd::CommandChangeTransform>(
      world, entity, oldPos, newPos, oldRot, newRot, oldScale, newScale));
  return true;
}

} // namespace

SceneHierarchyPanel::SceneHierarchyPanel() = default;

SceneHierarchyPanel::SceneHierarchyPanel(ecs::World &world)
    : context_(&world) {}

void SceneHierarchyPanel::SetContext(ecs::World &world) {
  context_ = &world;
  selection_context_ = ecs::INVALID_ENTITY;
}

void SceneHierarchyPanel::SetSelectedEntity(ecs::Entity entity) {
  if (!context_ || entity == ecs::INVALID_ENTITY) {
    selection_context_ = ecs::INVALID_ENTITY;
    return;
  }

  selection_context_ = context_->IsAlive(entity) ? entity : ecs::INVALID_ENTITY;
}

void SceneHierarchyPanel::ClearSelection() {
  selection_context_ = ecs::INVALID_ENTITY;
}

bool SceneHierarchyPanel::HasValidSelection() const {
  return context_ && selection_context_ != ecs::INVALID_ENTITY &&
         context_->IsAlive(selection_context_);
}

void SceneHierarchyPanel::ValidateSelection() {
  if (!HasValidSelection()) {
    selection_context_ = ecs::INVALID_ENTITY;
  }
}

bool SceneHierarchyPanel::IsRootEntity(ecs::Entity entity) const {
  if (!context_ || !context_->IsAlive(entity)) {
    return false;
  }

  if (!context_->HasComponent<ecs::RelationshipComponent>(entity)) {
    return true;
  }

  return context_->GetComponent<ecs::RelationshipComponent>(entity).Parent ==
         ecs::INVALID_ENTITY;
}

bool SceneHierarchyPanel::EntityMatchesFilter(ecs::Entity entity,
                                              const std::string &filter) const {
  if (!context_ || !context_->IsAlive(entity) || filter.empty()) {
    return filter.empty();
  }

  std::string tag = context_->HasComponent<ecs::TagComponent>(entity)
                        ? context_->GetComponent<ecs::TagComponent>(entity).tag
                        : ("Entity " + std::to_string(entity.GetIndex()));
  std::transform(tag.begin(), tag.end(), tag.begin(),
                 [](unsigned char c) { return (char)std::tolower(c); });
  return tag.find(filter) != std::string::npos;
}

bool SceneHierarchyPanel::HasMatchingDescendant(ecs::Entity entity,
                                                const std::string &filter) const {
  if (!context_ || filter.empty() || !context_->IsAlive(entity) ||
      !context_->HasComponent<ecs::RelationshipComponent>(entity)) {
    return false;
  }

  const auto &children =
      context_->GetComponent<ecs::RelationshipComponent>(entity).Children;
  for (auto child : children) {
    if (!context_->IsAlive(child)) {
      continue;
    }
    if (EntityMatchesFilter(child, filter) || HasMatchingDescendant(child, filter)) {
      return true;
    }
  }

  return false;
}

bool SceneHierarchyPanel::IsEntityInSubtree(ecs::Entity root,
                                            ecs::Entity candidate) const {
  if (!context_ || !context_->IsAlive(root) || candidate == ecs::INVALID_ENTITY) {
    return false;
  }

  if (root == candidate) {
    return true;
  }

  return context_->IsDescendantOf(candidate, root);
}

void SceneHierarchyPanel::OnImGuiRender() {
  if (!context_)
    return;

  ValidateSelection();

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
    if (IsRootEntity(entity)) {
      DrawEntityNode(entity, filter);
    }
  }

  // Right-click on blank space
  if (ImGui::BeginPopupContextWindow()) {
    if (ImGui::BeginMenu("Create")) {
      if (ImGui::MenuItem("Empty Entity")) {
        auto e = context_->CreateEntity();
        context_->AddComponent(e, ecs::TransformComponent{});
        context_->AddComponent(e, ecs::TagComponent{"Entity"});
        SetSelectedEntity(e);
      }

      if (ImGui::MenuItem("Sprite")) {
        auto e = context_->CreateEntity();
        context_->AddComponent(e, ecs::TransformComponent{});
        context_->AddComponent(e, ecs::TagComponent{"Sprite"});
        context_->AddComponent(e, ecs::SpriteComponent{});
        SetSelectedEntity(e);
      }

      if (ImGui::MenuItem("Spawn Point")) {
        auto e = context_->CreateEntity();
        context_->AddComponent(e, ecs::TransformComponent{});
        context_->AddComponent(e, ecs::TagComponent{"SpawnPoint"});
        SetSelectedEntity(e);
      }

      ImGui::EndMenu();
    }
    ImGui::EndPopup();
  }

  if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered() &&
      !ImGui::IsAnyItemHovered())
    selection_context_ = ecs::INVALID_ENTITY;

  ImGui::End();

  // 2. Inspector Window
  ImGui::Begin("Inspector");
  ValidateSelection();
  if (selection_context_) {
    DrawComponents(selection_context_);
  }

  ImGui::End();
}

void SceneHierarchyPanel::DrawEntityNode(ecs::Entity entity,
                                         const std::string &filter) {
  if (!context_ || !context_->IsAlive(entity)) {
    return;
  }

  const bool matchesFilter = EntityMatchesFilter(entity, filter);
  const bool hasMatchingDescendants = HasMatchingDescendant(entity, filter);
  if (!filter.empty() && !matchesFilter && !hasMatchingDescendants) {
    return;
  }

  std::string tag = context_->HasComponent<ecs::TagComponent>(entity)
                        ? context_->GetComponent<ecs::TagComponent>(entity).tag
                        : "Entity " + std::to_string(entity.GetIndex());

  bool hasChildren = context_->HasComponent<ecs::RelationshipComponent>(entity) &&
                     !context_->GetComponent<ecs::RelationshipComponent>(entity)
                          .Children.empty();
  ImGuiTreeNodeFlags flags =
      ((selection_context_ == entity) ? ImGuiTreeNodeFlags_Selected : 0) |
      ImGuiTreeNodeFlags_OpenOnArrow;
  if (!hasChildren) {
    flags |= ImGuiTreeNodeFlags_Leaf;
  }
  if (!filter.empty() && hasMatchingDescendants) {
    flags |= ImGuiTreeNodeFlags_DefaultOpen;
  }
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
          if (context_->IsAlive(droppedEntity)) {
            context_->SetParent(droppedEntity, entity);
          }
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
        std::filesystem::path prefabDir = EditorToolbar::GetAssetRoot();
        if (prefabDir.empty()) {
          prefabDir = "assets";
        }
        std::error_code ec;
        std::filesystem::create_directories(prefabDir, ec);
        std::filesystem::path filepath = prefabDir / (prefabTag + ".prefab");
        scene::PrefabSerializer::Serialize(*context_, entity, filepath.string());
    }

    ImGui::EndPopup();
  }

  if (opened) {
    if (context_->HasComponent<ecs::RelationshipComponent>(entity)) {
        auto& rc = context_->GetComponent<ecs::RelationshipComponent>(entity);
        for (auto child : rc.Children) {
            DrawEntityNode(child, filter);
        }
    }
    ImGui::TreePop();
  }

  if (entityDeleted) {
    if (IsEntityInSubtree(entity, selection_context_)) {
      selection_context_ = ecs::INVALID_ENTITY;
    }
    context_->DestroyEntity(entity);
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
  if (!context_ || !context_->IsAlive(entity)) {
    selection_context_ = ecs::INVALID_ENTITY;
    return;
  }

  if (context_->HasComponent<ecs::TagComponent>(entity)) {
    auto &tag = context_->GetComponent<ecs::TagComponent>(entity).tag;

    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    std::snprintf(buffer, sizeof(buffer), "%s", tag.c_str());
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

    showItem("Rigidbody 3D",
             [&]() { context_->AddComponent(entity, ecs::Rigidbody3DComponent{}); },
             !context_->HasComponent<ecs::Rigidbody3DComponent>(entity));

    showItem("Collider 3D",
             [&]() { context_->AddComponent(entity, ecs::Collider3DComponent{}); },
             !context_->HasComponent<ecs::Collider3DComponent>(entity));

    ImGui::EndPopup();
  }
  ImGui::PopItemWidth();
  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  if (context_->HasComponent<ecs::TransformComponent>(entity)) {
    DrawComponentControl<ecs::TransformComponent>("Transform", entity, [&]() {
      auto &tc = context_->GetComponent<ecs::TransformComponent>(entity);

      auto& precisionTool = PrecisionEditTool::Get();
      precisionTool.Update();

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

        precisionTool.SetMode(GizmoMode::Translate);
        if (DrawVec3Control("Position", tc.position)) {
          tc.position = precisionTool.SnapPosition(tc.position);
          changed = true;
        }

        Math::Vec3f euler = tc.rotation.ToEuler();
        Math::Vec3f degrees = { Math::RadiansToDegrees(euler.x), Math::RadiansToDegrees(euler.y), Math::RadiansToDegrees(euler.z) };
        if (DrawVec3Control("Rotation", degrees)) {
            precisionTool.SetMode(GizmoMode::Rotate);
            degrees = precisionTool.SnapRotation(degrees);
            tc.rotation = Math::Quatf::FromEuler(Math::DegreesToRadians(degrees.x), Math::DegreesToRadians(degrees.y), Math::DegreesToRadians(degrees.z));
            changed = true;
        }

        precisionTool.SetMode(GizmoMode::Scale);
        if (DrawVec3Control("Scale", tc.scale, 1.0f)) {
          tc.scale = precisionTool.SnapScale(tc.scale);
          changed = true;
        }

        ImGui::EndTable();
      }

      if (changed) {
        PushTransformCommandIfChanged(*context_, entity, oldPos, tc.position,
                                      oldRot, tc.rotation, oldScale, tc.scale);
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
        ImGui::Text("Pivot");
        ImGui::TableNextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::DragFloat2("##pivot", &sc.Pivot.x, 0.01f, 0.0f, 1.0f, "%.2f");
        ImGui::PopItemWidth();

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Sort");
        ImGui::TableNextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::DragInt2("##sort", &sc.SortingLayer, 0.1f);
        ImGui::PopItemWidth();

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Y Sort");
        ImGui::TableNextColumn();
        ImGui::Checkbox("##ysort", &sc.YSort);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("PPU");
        ImGui::TableNextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::DragFloat("##ppu", &sc.PixelsPerUnit, 0.25f, 1.0f, 256.0f, "%.2f");
        ImGui::PopItemWidth();

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Src Size");
        ImGui::TableNextColumn();
        ImGui::Checkbox("##usesourcesize", &sc.UseSourceSize);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Texture");
        ImGui::TableNextColumn();
        ImGui::PushItemWidth(-1);
        const char* textureLabel = sc.TexturePath.empty() ? "Texture Slot" : sc.TexturePath.c_str();
        ImGui::Button(textureLabel, ImVec2(-1, 0));
        ImGui::PopItemWidth();
        if (ImGui::BeginDragDropTarget()) {
          if (const ImGuiPayload *payload =
                  ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM")) {
            const wchar_t* path = static_cast<const wchar_t*>(payload->Data);
            std::filesystem::path texturePath(path);
            if (texturePath.has_extension()) {
              const auto extension = texturePath.extension().string();
              if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".bmp") {
                sc.TexturePath = MakeAssetRelativePath(texturePath);
                sc.texture = LoadPixelTexture(sc.TexturePath);
                GE_LOG_INFO("Loaded sprite texture: %s", sc.TexturePath.c_str());
              }
            }
          }
          ImGui::EndDragDropTarget();
        }

        ImGui::EndTable();
      }
    });
  }

  if (context_->HasComponent<ecs::TilemapComponent>(entity)) {
    DrawComponentControl<ecs::TilemapComponent>("Tilemap", entity, [&]() {
      auto& tilemap = context_->GetComponent<ecs::TilemapComponent>(entity);
      EnsureTilemapStorage(tilemap);

      int width = tilemap.Width;
      int height = tilemap.Height;

      if (ImGui::BeginTable("TilemapTable", 2,
                            ImGuiTableFlags_Resizable |
                                ImGuiTableFlags_BordersInnerV)) {
        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 110.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Tileset");
        ImGui::TableNextColumn();
        ImGui::PushItemWidth(-1);
        const char* textureLabel =
            tilemap.TilesetTexturePath.empty() ? "Drop Tileset Texture" : tilemap.TilesetTexturePath.c_str();
        ImGui::Button(textureLabel, ImVec2(-1, 0));
        ImGui::PopItemWidth();
        if (ImGui::BeginDragDropTarget()) {
          if (const ImGuiPayload* payload =
                  ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM")) {
            const wchar_t* path = static_cast<const wchar_t*>(payload->Data);
            std::filesystem::path texturePath(path);
            if (texturePath.has_extension()) {
              const auto extension = texturePath.extension().string();
              if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".bmp") {
                tilemap.TilesetTexturePath = MakeAssetRelativePath(texturePath);
                tilemap.TilesetTexture = LoadPixelTexture(tilemap.TilesetTexturePath);
              }
            }
          }
          ImGui::EndDragDropTarget();
        }

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Dimensions");
        ImGui::TableNextColumn();
        ImGui::PushItemWidth(-1);
        if (ImGui::DragInt2("##dimensions", &width, 1.0f, 1, 256)) {
          ResizeTilemapStorage(tilemap, width, height);
          RebuildTilemapNavigation(tilemap);
        }
        ImGui::PopItemWidth();

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Tile Size");
        ImGui::TableNextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::DragInt2("##tilesize", &tilemap.TileWidth, 1.0f, 1, 512);
        ImGui::PopItemWidth();

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Tiles/Row");
        ImGui::TableNextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::DragInt("##tilesperrow", &tilemap.TilesPerRow, 1.0f, 1, 512);
        ImGui::PopItemWidth();

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Pixel Scale");
        ImGui::TableNextColumn();
        ImGui::PushItemWidth(-1);
        if (ImGui::DragFloat("##pixelsperunit", &tilemap.PixelsPerUnit, 0.25f, 1.0f, 256.0f, "%.2f")) {
          RebuildTilemapNavigation(tilemap);
        }
        ImGui::PopItemWidth();

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Chunk");
        ImGui::TableNextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::DragInt2("##chunksize", &tilemap.ChunkWidth, 1.0f, 1, 256);
        ImGui::PopItemWidth();

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Auto Nav");
        ImGui::TableNextColumn();
        if (ImGui::Checkbox("##autobuildnav", &tilemap.AutoBuildNavigation)) {
          if (tilemap.AutoBuildNavigation) {
            RebuildTilemapNavigation(tilemap);
          }
        }

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Layers");
        ImGui::TableNextColumn();
        if (ImGui::Button("Add Layer")) {
          ecs::TilemapLayer layer;
          layer.Name = "Layer " + std::to_string(tilemap.Layers.size() + 1);
          layer.Tiles.resize(static_cast<size_t>(tilemap.Width * tilemap.Height), -1);
          tilemap.Layers.push_back(std::move(layer));
          tilemap_active_layer_ = static_cast<int>(tilemap.Layers.size()) - 1;
        }
        ImGui::SameLine();
        if (ImGui::Button("Bake Navigation")) {
          RebuildTilemapNavigation(tilemap);
        }

        ImGui::EndTable();
      }

      if (!tilemap.Layers.empty()) {
        tilemap_active_layer_ =
            Math::Clamp(tilemap_active_layer_, 0, static_cast<int>(tilemap.Layers.size()) - 1);
        auto& activeLayer = tilemap.Layers[static_cast<size_t>(tilemap_active_layer_)];

        ImGui::Separator();
        ImGui::Text("Layer Authoring");
        if (ImGui::BeginCombo("Active Layer", activeLayer.Name.c_str())) {
          for (int i = 0; i < static_cast<int>(tilemap.Layers.size()); ++i) {
            const bool selected = tilemap_active_layer_ == i;
            if (ImGui::Selectable(tilemap.Layers[static_cast<size_t>(i)].Name.c_str(), selected)) {
              tilemap_active_layer_ = i;
            }
            if (selected) {
              ImGui::SetItemDefaultFocus();
            }
          }
          ImGui::EndCombo();
        }

        char layerName[64];
        std::snprintf(layerName, sizeof(layerName), "%s", activeLayer.Name.c_str());
        if (ImGui::InputText("Layer Name", layerName, sizeof(layerName))) {
          activeLayer.Name = layerName;
        }

        ImGui::Checkbox("Visible", &activeLayer.Visible);
        ImGui::SameLine();
        if (ImGui::Checkbox("Collision Layer", &activeLayer.CollisionLayer)) {
          RebuildTilemapNavigation(tilemap);
        }
        ImGui::DragFloat("Z Offset", &activeLayer.ZOffset, 0.01f, -10.0f, 10.0f, "%.2f");

        ImGui::Spacing();
        ImGui::Text("Palette");
        if (tilemap.TilePalette.empty()) {
          tilemap.TilePalette.push_back(Math::Vec4f{1.0f, 1.0f, 1.0f, 1.0f});
        }

        if (ImGui::Button("Add Palette Color")) {
          tilemap.TilePalette.push_back(Math::Vec4f{1.0f, 1.0f, 1.0f, 1.0f});
        }
        ImGui::SameLine();
        if (ImGui::Button("Fill Layer")) {
          std::fill(activeLayer.Tiles.begin(), activeLayer.Tiles.end(), tilemap_selected_tile_);
          RebuildTilemapNavigation(tilemap);
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear Layer")) {
          std::fill(activeLayer.Tiles.begin(), activeLayer.Tiles.end(), -1);
          RebuildTilemapNavigation(tilemap);
        }

        for (size_t i = 0; i < tilemap.TilePalette.size(); ++i) {
          ImGui::PushID(static_cast<int>(i));
          if (i > 0) {
            ImGui::SameLine();
          }
          if (ImGui::ColorButton("##palette", ImVec4(tilemap.TilePalette[i].x, tilemap.TilePalette[i].y,
                                                      tilemap.TilePalette[i].z, tilemap.TilePalette[i].w),
                                 ImGuiColorEditFlags_NoTooltip, ImVec2(20, 20))) {
            tilemap_selected_tile_ = static_cast<int>(i);
          }
          ImGui::PopID();
        }

        tilemap_selected_tile_ = Math::Clamp(
            tilemap_selected_tile_, -1, static_cast<int>(tilemap.TilePalette.size()) - 1);
        ImGui::Text("Selected Tile: %d", tilemap_selected_tile_);

        if (tilemap.Width > 0 && tilemap.Height > 0) {
          ImGui::Spacing();
          ImGui::Text("Paint Grid");
          const float cellSize = 24.0f;
          const int displayWidth = (std::min)(tilemap.Width, 24);
          const int displayHeight = (std::min)(tilemap.Height, 24);
          bool gridEdited = false;

          if (displayWidth != tilemap.Width || displayHeight != tilemap.Height) {
            ImGui::TextDisabled("Showing first %dx%d cells for quick editing.", displayWidth, displayHeight);
          }

          for (int y = 0; y < displayHeight; ++y) {
            for (int x = 0; x < displayWidth; ++x) {
              const int tileIndex = y * tilemap.Width + x;
              ImGui::PushID(tileIndex);

              const int tileValue = activeLayer.Tiles[static_cast<size_t>(tileIndex)];
              ImVec4 buttonColor = ImVec4(0.15f, 0.16f, 0.18f, 1.0f);
              if (tileValue >= 0 && tileValue < static_cast<int>(tilemap.TilePalette.size())) {
                const auto& color = tilemap.TilePalette[static_cast<size_t>(tileValue)];
                buttonColor = ImVec4(color.x, color.y, color.z, color.w);
              }

              ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
              ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(
                  (std::min)(buttonColor.x + 0.1f, 1.0f),
                  (std::min)(buttonColor.y + 0.1f, 1.0f),
                  (std::min)(buttonColor.z + 0.1f, 1.0f),
                  buttonColor.w));
              ImGui::PushStyleColor(ImGuiCol_ButtonActive, buttonColor);

              char label[8];
              std::snprintf(label, sizeof(label), "%d", tileValue);
              if (tileValue < 0) {
                std::snprintf(label, sizeof(label), ".");
              }

              if (ImGui::Button(label, ImVec2(cellSize, cellSize))) {
                activeLayer.Tiles[static_cast<size_t>(tileIndex)] = tilemap_selected_tile_;
                gridEdited = true;
                if (activeLayer.CollisionLayer) {
                  RebuildTilemapNavigation(tilemap);
                }
              }
              if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                activeLayer.Tiles[static_cast<size_t>(tileIndex)] = tilemap_selected_tile_;
                gridEdited = true;
              }
              if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
                activeLayer.Tiles[static_cast<size_t>(tileIndex)] = -1;
                gridEdited = true;
              }

              ImGui::PopStyleColor(3);
              ImGui::PopID();
              if (x + 1 < displayWidth) {
                ImGui::SameLine();
              }
            }
          }

          if (gridEdited && activeLayer.CollisionLayer) {
            RebuildTilemapNavigation(tilemap);
          }
        }
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
        ImGui::TableNextColumn();
        ImGui::TextWrapped("%s", mc.MeshPath.empty() ? "None" : mc.MeshPath.c_str());

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

  if (context_->HasComponent<ecs::Rigidbody3DComponent>(entity)) {
    DrawComponentControl<ecs::Rigidbody3DComponent>("Rigidbody 3D", entity, [&]() {
      auto &rb = context_->GetComponent<ecs::Rigidbody3DComponent>(entity);
      
      const char* bodyTypes[] = { "Static", "Kinematic", "Dynamic" };
      int bodyTypeIndex = (int)rb.MotionType;
      if (ImGui::Combo("Motion Type", &bodyTypeIndex, bodyTypes, 3)) {
        rb.MotionType = (ecs::Rigidbody3DMotionType)bodyTypeIndex;
      }

      ImGui::DragFloat("Mass", &rb.Mass, 0.1f, 0.01f, 1000.0f);
      ImGui::DragFloat("Linear Damping", &rb.LinearDamping, 0.01f, 0.0f, 1.0f);
      ImGui::DragFloat("Angular Damping", &rb.AngularDamping, 0.01f, 0.0f, 1.0f);
      ImGui::Checkbox("Allow Sleeping", &rb.AllowSleeping);
    });
  }

  if (context_->HasComponent<ecs::Collider3DComponent>(entity)) {
    DrawComponentControl<ecs::Collider3DComponent>("Collider 3D", entity, [&]() {
      auto &cc = context_->GetComponent<ecs::Collider3DComponent>(entity);
      
      const char* shapes[] = { "Box", "Sphere", "Capsule" };
      int shapeIndex = (int)cc.ShapeType;
      if (ImGui::Combo("Shape Type", &shapeIndex, shapes, 3)) {
        cc.ShapeType = (ecs::Collider3DShapeType)shapeIndex;
      }

      if (cc.ShapeType == ecs::Collider3DShapeType::Box) {
        ImGui::DragFloat3("Half Extents", &cc.BoxHalfExtents.x, 0.1f, 0.01f, 100.0f);
      } else if (cc.ShapeType == ecs::Collider3DShapeType::Sphere) {
        ImGui::DragFloat("Radius", &cc.SphereRadius, 0.1f, 0.01f, 100.0f);
      } else if (cc.ShapeType == ecs::Collider3DShapeType::Capsule) {
        ImGui::DragFloat("Radius", &cc.CapsuleRadius, 0.1f, 0.01f, 100.0f);
        ImGui::DragFloat("Half Height", &cc.CapsuleHalfHeight, 0.1f, 0.01f, 100.0f);
      }

      ImGui::DragFloat3("Offset", &cc.Offset.x, 0.1f);
      ImGui::Checkbox("Is Trigger", &cc.IsTrigger);
      ImGui::DragFloat("Friction", &cc.Friction, 0.1f, 0.0f, 1.0f);
      ImGui::DragFloat("Restitution", &cc.Restitution, 0.1f, 0.0f, 1.0f);
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
