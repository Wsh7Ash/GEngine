#include "SceneHierarchyPanel.h"
#include <imgui.h>
#include "../ecs/components/TransformComponent.h"
#include "../ecs/components/SpriteComponent.h"
#include "../ecs/components/TagComponent.h"

namespace ge {
namespace editor {

    SceneHierarchyPanel::SceneHierarchyPanel() = default;

    SceneHierarchyPanel::SceneHierarchyPanel(ecs::World& world)
        : context_(&world) { }

    void SceneHierarchyPanel::SetContext(ecs::World& world)
    {
        context_ = &world;
        selection_context_ = ecs::Entity(); // Reset selection
    }

    void SceneHierarchyPanel::OnImGuiRender()
    {
        if (!context_) return;

        // 1. Hierarchy Window
        ImGui::Begin("Scene Hierarchy");

        // Iterate through all entities that have a Transform (as a proxy for "alive")
        // In a real system, we'd have a list of active entities.
        for (uint32_t i = 0; i < 10000; ++i)
        {
            ecs::Entity entity(i);
            if (context_->HasComponent<ecs::TransformComponent>(entity))
            {
                DrawEntityNode(entity);
            }
        }

        if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
            selection_context_ = ecs::Entity();

        ImGui::End();

        // 2. Inspector Window
        ImGui::Begin("Inspector");
        if (selection_context_)
        {
            DrawComponents(selection_context_);
        }
        ImGui::End();
    }

    void SceneHierarchyPanel::DrawEntityNode(ecs::Entity entity)
    {
        std::string tag = context_->HasComponent<ecs::TagComponent>(entity) 
            ? context_->GetComponent<ecs::TagComponent>(entity).tag 
            : "Entity " + std::to_string(entity.GetIndex());

        ImGuiTreeNodeFlags flags = ((selection_context_ == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
        flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
        bool opened = ImGui::TreeNodeEx((void*)(uint64_t)entity.GetIndex(), flags, tag.c_str());
        
        if (ImGui::IsItemClicked())
        {
            selection_context_ = entity;
        }

        if (opened)
        {
            // Placeholder for children
            ImGui::TreePop();
        }
    }

    void SceneHierarchyPanel::DrawComponents(ecs::Entity entity)
    {
        if (context_->HasComponent<ecs::TagComponent>(entity))
        {
            auto& tag = context_->GetComponent<ecs::TagComponent>(entity).tag;
            char buffer[256];
            memset(buffer, 0, sizeof(buffer));
            snprintf(buffer, sizeof(buffer), "%s", tag.c_str());
            if (ImGui::InputText("Tag", buffer, sizeof(buffer)))
            {
                tag = std::string(buffer);
            }
        }

        ImGui::Separator();

        if (context_->HasComponent<ecs::TransformComponent>(entity))
        {
            if (ImGui::TreeNodeEx((void*)typeid(ecs::TransformComponent).hash_code(), ImGuiTreeNodeFlags_DefaultOpen, "Transform"))
            {
                auto& tc = context_->GetComponent<ecs::TransformComponent>(entity);
                ImGui::DragFloat3("Position", &tc.position.x, 0.1f);
                
                // Rotation (simple euler approximation for now)
                Math::Vec3f rotation = { 0, 0, 0 }; // We don't store eulers, but we could convert from quat
                if (ImGui::DragFloat3("Rotation", &rotation.x, 0.1f))
                {
                    // Update quat from eulers
                }

                ImGui::DragFloat3("Scale", &tc.scale.x, 0.1f);
                ImGui::TreePop();
            }
        }

        if (context_->HasComponent<ecs::SpriteComponent>(entity))
        {
            if (ImGui::TreeNodeEx((void*)typeid(ecs::SpriteComponent).hash_code(), ImGuiTreeNodeFlags_DefaultOpen, "Sprite"))
            {
                auto& sc = context_->GetComponent<ecs::SpriteComponent>(entity);
                ImGui::ColorEdit4("Color", &sc.color.x);
                ImGui::TreePop();
            }
        }
    }

} // namespace editor
} // namespace ge
