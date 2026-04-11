#pragma once
#include <filesystem>
#include <string>

namespace ge {
namespace ecs {
class World;
}
namespace editor {
class ContentBrowserPanel {
public:
  ContentBrowserPanel();
  void OnImGuiRender();
  void SetContext(ecs::World &world) { context_ = &world; }
  void SetBaseDirectory(const std::filesystem::path& directory);

private:
  void DrawDirectoryTree(const std::filesystem::path &directory);
  void RefreshAssets(bool force = false);

  std::filesystem::path base_dir_;
  std::filesystem::path cur_dir_;
  ecs::World *context_ = nullptr;
  bool first_render_ = true;
  std::filesystem::file_time_type asset_stamp_{};
  bool asset_stamp_valid_ = false;
};

} // namespace editor
} // namespace ge
