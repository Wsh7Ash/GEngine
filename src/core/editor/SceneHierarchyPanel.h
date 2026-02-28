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

private:
  ecs::World *context_ = nullptr;
  ecs::Entity selection_context_; // Default constructed handle (Null)
};

} // namespace editor
} // namespace ge
