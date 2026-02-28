#include "ContentBrowserPanel.h"
#include <imgui.h>

namespace ge {
namespace editor {
static const std::filesystem::path s_AssetsDirectory = "../assets";

ContentBrowserPanel::ContentBrowserPanel() : cur_dir_(s_AssetsDirectory) {}

static std::filesystem::path GetAssetPath() {
  std::vector<std::string> paths = {"./", "../", "../../", "../../../"};
  for (auto p : paths) {
    if (std::filesystem::exists(p + "assets")) {
      return std::filesystem::path(p + "assets");
    }
  }
  return "";
}

// TODO: Add file selection
void ContentBrowserPanel::OnImGuiRender() {
  float padding = 16.0f;
  float thumbnailSize = 128.0f;
  float cellSize = thumbnailSize + padding;
  float panelWidth = ImGui::GetContentRegionAvail().x;
  int columnCount = (int)(panelWidth / cellSize);

  columnCount = std::max(1, columnCount);
  ImGui::Columns(columnCount, 0, false);

  ImGui::Begin("Content Browser");
  if (cur_dir_ != std::filesystem::path(s_AssetsDirectory)) {
    if (ImGui::Selectable("<-Back")) {
      cur_dir_ = cur_dir_.parent_path();
    }
  }

  if (cur_dir_.empty() || !std::filesystem::exists(cur_dir_)) {
    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
                       "Assets directory not found!");
    ImGui::End();
    return;
  }

  for (auto &directoryEntity : std::filesystem::directory_iterator(cur_dir_)) {
    const auto &path = directoryEntity.path();
    auto relativePath = std::filesystem::relative(path, s_AssetsDirectory);
    std::string filenameString = path.filename().string();
    if (directoryEntity.is_directory()) {
      if (ImGui::Selectable(filenameString.c_str())) {
        cur_dir_ /= path.filename();
      }
    } else {
      if (ImGui::Selectable(filenameString.c_str())) {
        // TODO: Open file
      }
    }
  }

  for (auto &directoryEntry : std::filesystem::directory_iterator(cur_dir_)) {
    const auto &path = directoryEntry.path();
    std::string filenameString = path.filename().string();
    ImGui::PushID(filenameString.c_str());
    ImGui::Button(filenameString.c_str(), ImVec2(thumbnailSize, thumbnailSize));

    // Drag and drop
    if (ImGui::BeginDragDropSource()) {
      const wchar_t *itemPath = path.c_str();
      ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", itemPath,
                                (wcslen(itemPath) + 1) * sizeof(wchar_t));
      ImGui::TextUnformatted(filenameString.c_str());
      ImGui::EndDragDropSource();
    }

    // Directory navigation (double click)
    if (directoryEntry.is_directory()) {
      if (ImGui::IsItemHovered && ImGui::IsMouseDoubleClicked(0)) {
        cur_dir_ /= path.filename();
      }
    }
    ImGui::TextWrapped(filenameString.c_str());
    ImGui::NextColumn();
    ImGui::PopID();
  }

  ImGui::Columns(1);

  ImGui::End();
}
} // namespace editor
} // namespace ge