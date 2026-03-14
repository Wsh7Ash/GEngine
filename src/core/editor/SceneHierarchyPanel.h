#pragma once

#include "../ecs/Entity.h"
#include "../ecs/World.h"
#include <memory>

namespace ge {
namespace editor {

/**
 * @brief Panel for displaying the entity hierarchy and editing component
 * properties.
 */
class SceneHierarchyPanel {
public:
  SceneHierarchyPanel();
  SceneHierarchyPanel(ecs::World &world);

  void SetContext(ecs::World &world);

  void OnImGuiRender();

  ecs::Entity GetSelectedEntity() const { return selection_context_; }
  void SetSelectedEntity(ecs::Entity entity) { selection_context_ = entity; }

private:
  void DrawEntityNode(ecs::Entity entity);
  void DrawComponents(ecs::Entity entity);

  template <typename T, typename UIFunction>
  void DrawComponentControl(const std::string &name, ecs::Entity entity,
                            UIFunction uiFunction);

private:
  ecs::World *context_ = nullptr;
  ecs::Entity selection_context_; // Default constructed handle (Null)
  char search_filter_[256] = "";
  char component_search_[128] = "";
  std::string component_clipboard_;
};

} // namespace editor
} // namespace ge
