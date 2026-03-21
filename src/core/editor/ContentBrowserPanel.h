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

private:
  void DrawDirectoryTree(const std::filesystem::path &directory);

  std::filesystem::path base_dir_;
  std::filesystem::path cur_dir_;
  ecs::World *context_ = nullptr;
  bool first_render_ = true;
};

} // namespace editor
} // namespace ge