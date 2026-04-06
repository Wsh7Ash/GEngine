#pragma once

// ================================================================
//  PrefabOverridesPanel.h
//  Editor panel for visualizing and managing prefab overrides.
// ================================================================

#include <imgui.h>
#include <string>
#include <memory>

namespace ge {
namespace ecs {
class World;
class Entity;
}
namespace editor {

class PrefabOverridesPanel {
public:
    PrefabOverridesPanel() = default;
    ~PrefabOverridesPanel() = default;
    
    void SetContext(ecs::World* world) { world_ = world; }
    
    void OnImGuiRender();
    
    void ShowForEntity(ecs::Entity entity);
    
private:
    void RenderOverrideList();
    void RenderDiffViewer();
    void RenderComponentDiff(const char* componentName, const std::vector<scene::FieldDiff>& diffs);
    
    ecs::World* world_ = nullptr;
    ecs::Entity selectedEntity_ = ecs::INVALID_ENTITY;
    bool showPanel_ = false;
};

} // namespace editor
} // namespace ge