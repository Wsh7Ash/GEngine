#pragma once
#include <filesystem>

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
  std::filesystem::path cur_dir_;
  ecs::World *context_ = nullptr;
};

} // namespace editor
} // namespace ge