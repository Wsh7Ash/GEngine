#include "ContentBrowserPanel.h"
#include "EditorPaths.h"
#include "EditorToolbar.h"
#include "../scene/SceneSerializer.h"
#include "VSCodeUtility.h"
#include <algorithm>
#include <filesystem>
#include <imgui.h>
#include <string>
#include <vector>
#include "../assets/AssetManager.h"
#include "../assets/AssetImporter.h"
#include "../debug/log.h"

namespace ge {
namespace editor {

static const char* GetFileIcon(const std::filesystem::path& path) {
  if (std::filesystem::is_directory(path)) return "[D]";
  auto ext = path.extension().string();
  if (ext == ".png" || ext == ".jpg" || ext == ".bmp") return "[IMG]";
  if (ext == ".json") return "[SCN]";
  if (ext == ".cpp" || ext == ".h") return "[CPP]";
  if (ext == ".glsl" || ext == ".vert" || ext == ".frag") return "[SHD]";
  if (ext == ".obj" || ext == ".fbx") return "[MDL]";
  if (ext == ".wav" || ext == ".mp3") return "[AUD]";
  return "[F]";
}

ContentBrowserPanel::ContentBrowserPanel() {
  base_dir_ = EditorPaths::ResolveAssetRoot();
  cur_dir_ = base_dir_;
}

void ContentBrowserPanel::SetBaseDirectory(const std::filesystem::path& directory) {
  base_dir_ = EditorPaths::NormalizePath(directory);
  cur_dir_ = base_dir_;
  first_render_ = true;
  asset_stamp_valid_ = false;
}

void ContentBrowserPanel::RefreshAssets(bool force) {
  if (base_dir_.empty()) {
    base_dir_ = EditorPaths::ResolveAssetRoot();
  }

  std::error_code ec;
  if (base_dir_.empty() || !std::filesystem::exists(base_dir_, ec)) {
    return;
  }

  const auto newStamp = std::filesystem::last_write_time(base_dir_, ec);
  if (!force && asset_stamp_valid_ && !ec && newStamp == asset_stamp_) {
    return;
  }

  assets::AssetImporter::ScanDirectory(base_dir_);
  if (!ec) {
    asset_stamp_ = newStamp;
    asset_stamp_valid_ = true;
  }

  if (cur_dir_.empty() || !std::filesystem::exists(cur_dir_, ec)) {
    cur_dir_ = base_dir_;
  }
}

void ContentBrowserPanel::DrawDirectoryTree(const std::filesystem::path &directory) {
  std::error_code ec;
  if (!std::filesystem::exists(directory, ec) || !std::filesystem::is_directory(directory, ec))
    return;

  std::vector<std::filesystem::directory_entry> subDirectories;
  for (auto &entry : std::filesystem::directory_iterator(directory, ec)) {
    if (ec || !entry.is_directory(ec)) continue;
    subDirectories.push_back(entry);
  }

  std::sort(subDirectories.begin(), subDirectories.end(),
            [](const auto& a, const auto& b) {
              return a.path().filename().string() < b.path().filename().string();
            });

  for (auto &entry : subDirectories) {

    std::string name = entry.path().filename().string();
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
                               ImGuiTreeNodeFlags_SpanAvailWidth;
    if (cur_dir_ == entry.path())
      flags |= ImGuiTreeNodeFlags_Selected;

    // Check if this directory has subdirectories
    bool hasSubDirs = false;
    std::error_code subEc;
    for (auto &sub : std::filesystem::directory_iterator(entry.path(), subEc)) {
      if (subEc) continue;
      if (sub.is_directory(subEc)) { hasSubDirs = true; break; }
    }
    if (!hasSubDirs)
      flags |= ImGuiTreeNodeFlags_Leaf;

    bool opened = ImGui::TreeNodeEx(name.c_str(), flags);

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
      cur_dir_ = entry.path();
    }

    if (opened) {
      DrawDirectoryTree(entry.path());
      ImGui::TreePop();
    }
  }
}

void ContentBrowserPanel::OnImGuiRender() {
  ImGui::Begin("Content Browser");

  if (base_dir_.empty()) {
    SetBaseDirectory(EditorToolbar::GetAssetRoot());
  }

  if (base_dir_.empty() || !std::filesystem::exists(base_dir_)) {
    ImGui::TextColored(ImVec4(0.85f, 0.25f, 0.25f, 1.0f),
                       "Assets directory not found!");
    ImGui::End();
    return;
  }

  if (first_render_) {
    RefreshAssets(true);
    first_render_ = false;
  } else {
    RefreshAssets(false);
  }

  float panelWidth = ImGui::GetContentRegionAvail().x;
  float treeWidth = panelWidth * 0.25f;
  if (treeWidth < 120.0f) treeWidth = 120.0f;

  // ── Left Pane: Folder Tree ──
  ImGui::BeginChild("FolderTree", ImVec2(treeWidth, 0), true);

  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.00f, 0.71f, 0.85f, 1.00f));
  ImGui::Text("Folders");
  ImGui::PopStyleColor();
  ImGui::SameLine();
  if (ImGui::SmallButton("Refresh")) {
    RefreshAssets(true);
  }
  ImGui::SameLine();
  if (ImGui::SmallButton("Up") && cur_dir_ != base_dir_) {
    const auto parent = cur_dir_.parent_path();
    if (!parent.empty() && parent.string().find(base_dir_.string()) == 0) {
      cur_dir_ = parent;
    }
  }
  ImGui::Separator();

  // Root node
  ImGuiTreeNodeFlags rootFlags = ImGuiTreeNodeFlags_OpenOnArrow |
                                 ImGuiTreeNodeFlags_DefaultOpen |
                                 ImGuiTreeNodeFlags_SpanAvailWidth;
  if (cur_dir_ == base_dir_)
    rootFlags |= ImGuiTreeNodeFlags_Selected;

  bool rootOpen = ImGui::TreeNodeEx("Assets", rootFlags);
  if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
    cur_dir_ = base_dir_;
  }
  if (rootOpen) {
    DrawDirectoryTree(base_dir_);
    ImGui::TreePop();
  }

  ImGui::EndChild();

  ImGui::SameLine();

  // ── Right Pane: File Grid ──
  ImGui::BeginChild("FileGrid", ImVec2(0, 0), true);

  // Breadcrumb / current path
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.60f, 0.60f, 0.60f, 1.00f));
  std::error_code ec;
  std::string relPath = std::filesystem::relative(cur_dir_, base_dir_, ec).string();
  if (relPath == ".") relPath = "Assets";
  else relPath = "Assets/" + relPath;
  ImGui::Text("%s", relPath.c_str());
  ImGui::PopStyleColor();
  ImGui::Separator();
  ImGui::Spacing();

  float padding = 16.0f;
  float thumbnailSize = 80.0f;
  float cellSize = thumbnailSize + padding;
  float gridWidth = ImGui::GetContentRegionAvail().x;
  int columnCount = (int)(gridWidth / cellSize);
  columnCount = std::max(1, columnCount);

  ImGui::Columns(columnCount, 0, false);

  std::vector<std::filesystem::directory_entry> entries;
  for (auto &directoryEntry : std::filesystem::directory_iterator(cur_dir_, ec)) {
    if (ec) continue;
    entries.push_back(directoryEntry);
  }

  std::sort(entries.begin(), entries.end(),
            [](const auto& a, const auto& b) {
              std::error_code leftEc;
              std::error_code rightEc;
              const bool leftDir = a.is_directory(leftEc);
              const bool rightDir = b.is_directory(rightEc);
              if (leftDir != rightDir) {
                return leftDir > rightDir;
              }
              return a.path().filename().string() < b.path().filename().string();
            });

  for (auto &directoryEntry : entries) {
    const auto &path = directoryEntry.path();
    if (path.extension() == ".geasset") continue;

    std::string filenameString = path.filename().string();
    ImGui::PushID(filenameString.c_str());

    bool isDirectory = directoryEntry.is_directory(ec);
    const char* icon = GetFileIcon(path);

    // Color-code buttons
    if (isDirectory) {
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.00f, 0.35f, 0.45f, 0.50f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.00f, 0.55f, 0.67f, 0.70f));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.00f, 0.71f, 0.85f, 0.80f));
    } else {
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.14f, 0.14f, 0.17f, 1.00f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.20f, 0.25f, 1.00f));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.25f, 0.30f, 1.00f));
    }

    ImGui::Button(icon, ImVec2(thumbnailSize, thumbnailSize));
    ImGui::PopStyleColor(3);

    // Drag and drop source
    if (ImGui::BeginDragDropSource()) {
      assets::AssetHandle handle = assets::AssetManager::GetHandleFromPath(path);
      if (handle) {
        ImGui::SetDragDropPayload("ASSET_HANDLE", &handle, sizeof(assets::AssetHandle));
        ImGui::Text("Asset: %s", filenameString.c_str());
      } else {
        const wchar_t *itemPath = path.c_str();
        ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", itemPath,
                                  (wcslen(itemPath) + 1) * sizeof(wchar_t));
        ImGui::TextUnformatted(filenameString.c_str());
      }
      ImGui::EndDragDropSource();
    }

    // Double-click actions
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
      if (isDirectory) {
        cur_dir_ = path;
      } else if (EditorPaths::LooksLikeSceneAsset(path) && context_) {
        EditorToolbar::LoadScene(path);
      } else if (path.extension() == ".cpp" || path.extension() == ".h") {
        VSCodeUtility::OpenInVSCode(std::filesystem::absolute(path, ec).string());
      }
    }

    // Context Menu
    if (ImGui::BeginPopupContextItem()) {
      if (ImGui::MenuItem("Open in Explorer")) {
        VSCodeUtility::OpenInExplorer(std::filesystem::absolute(path, ec).string());
      }
      if (ImGui::MenuItem("Copy Path")) {
        ImGui::SetClipboardText(
            std::filesystem::absolute(path, ec).string().c_str());
      }
      if (!isDirectory && EditorPaths::LooksLikeSceneAsset(path)) {
        if (ImGui::MenuItem("Load Scene")) {
          EditorToolbar::LoadScene(path);
        }
      }
      if (!isDirectory && (path.extension() == ".cpp" || path.extension() == ".h")) {
        if (ImGui::MenuItem("Edit in VS Code")) {
          VSCodeUtility::OpenInVSCode(std::filesystem::absolute(path, ec).string());
        }
      }
      ImGui::EndPopup();
    }

    // Label
    if (isDirectory) {
      ImGui::TextColored(ImVec4(0.00f, 0.71f, 0.85f, 1.00f), "%s",
                         filenameString.c_str());
    } else {
      ImGui::TextWrapped("%s", filenameString.c_str());
    }

    ImGui::NextColumn();
    ImGui::PopID();
  }

  ImGui::Columns(1);
  ImGui::EndChild();

  ImGui::End();
}
} // namespace editor
} // namespace ge
