#include "ContentBrowserPanel.h"
#include <imgui.h>

namespace ge {
namespace editor {
// static const std::filesystem::path s_AssetsDirectory = "../assets";
// must be define first!
static std::filesystem::path GetAssetPath() {
  std::vector<std::string> paths = {"./", "../", "../../", "../../../"};
  for (auto p : paths) {
    if (std::filesystem::exists(p + "assets")) {
      return std::filesystem::path(p + "assets");
    }
  }
  return "";
}

ContentBrowserPanel::ContentBrowserPanel() { cur_dir_ = GetAssetPath(); }

// TODO: Add file selection
void ContentBrowserPanel::OnImGuiRender() {
  float padding = 16.0f;
  float thumbnailSize = 96.0f;
  float cellSize = thumbnailSize + padding;
  float panelWidth = ImGui::GetContentRegionAvail().x;
  int columnCount = (int)(panelWidth / cellSize);
  columnCount = std::max(1, columnCount);

  ImGui::Begin("Content Browser");

  // Styled title
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.00f, 0.71f, 0.85f, 1.00f));
  ImGui::Text("Assets");
  ImGui::PopStyleColor();
  ImGui::Separator();
  ImGui::Spacing();

  auto baseAsset = GetAssetPath();

  // Back button with accent styling
  if (cur_dir_ != baseAsset) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.00f, 0.45f, 0.55f, 0.40f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                          ImVec4(0.00f, 0.55f, 0.67f, 0.60f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                          ImVec4(0.00f, 0.35f, 0.45f, 0.80f));
    if (ImGui::Button("<- Back", ImVec2(-1, 0))) {
      cur_dir_ = cur_dir_.parent_path();
    }
    ImGui::PopStyleColor(3);
    ImGui::Spacing();
  }

  if (cur_dir_.empty() || !std::filesystem::exists(cur_dir_)) {
    ImGui::TextColored(ImVec4(0.85f, 0.25f, 0.25f, 1.0f),
                       "Assets directory not found!");
    ImGui::End();
    return;
  }

  ImGui::Columns(columnCount, 0, false);

  for (auto &directoryEntry : std::filesystem::directory_iterator(cur_dir_)) {
    const auto &path = directoryEntry.path();
    std::string filenameString = path.filename().string();
    ImGui::PushID(filenameString.c_str());

    bool isDirectory = directoryEntry.is_directory();

    // Color-code: teal for folders, neutral for files
    if (isDirectory) {
      ImGui::PushStyleColor(ImGuiCol_Button,
                            ImVec4(0.00f, 0.35f, 0.45f, 0.50f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                            ImVec4(0.00f, 0.55f, 0.67f, 0.70f));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                            ImVec4(0.00f, 0.71f, 0.85f, 0.80f));
    } else {
      ImGui::PushStyleColor(ImGuiCol_Button,
                            ImVec4(0.14f, 0.14f, 0.17f, 1.00f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                            ImVec4(0.20f, 0.20f, 0.25f, 1.00f));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                            ImVec4(0.25f, 0.25f, 0.30f, 1.00f));
    }

    ImGui::Button(filenameString.c_str(), ImVec2(thumbnailSize, thumbnailSize));
    ImGui::PopStyleColor(3);

    // Drag and drop
    if (ImGui::BeginDragDropSource()) {
      const wchar_t *itemPath = path.c_str();
      ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", itemPath,
                                (wcslen(itemPath) + 1) * sizeof(wchar_t));
      ImGui::TextUnformatted(filenameString.c_str());
      ImGui::EndDragDropSource();
    }

    // Directory navigation (double click)
    if (isDirectory) {
      if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
        cur_dir_ /= path.filename();
      }
    }

    // Label with color coding
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
  ImGui::End();
}
} // namespace editor
} // namespace ge