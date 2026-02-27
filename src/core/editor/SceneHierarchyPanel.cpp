#include "SceneHierarchyPanel.h"
#include <imgui.h>
#include "../ecs/components/TransformComponent.h"
#include "../ecs/components/SpriteComponent.h"
#include "../ecs/components/TagComponent.h"
#include "../ecs/components/NativeScriptComponent.h"

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

        // Iterate through all entities (using a fixed limit for now, but checking IsAlive)
        for (uint32_t i = 0; i < 10000; ++i)
        {
            ecs::Entity entity(i);
            if (context_->IsAlive(entity))
            {
                DrawEntityNode(entity);
            }
        }

        // Right-click on blank space
        if (ImGui::BeginPopupContextWindow())
        {
            if (ImGui::MenuItem("Create Empty Entity"))
            {
                auto e = context_->CreateEntity();
                context_->AddComponent(e, ecs::TransformComponent{});
                context_->AddComponent(e, ecs::TagComponent{ "Entity" });
            }
            ImGui::EndPopup();
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

        bool entityDeleted = false;
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Delete Entity"))
                entityDeleted = true;

            ImGui::EndPopup();
        }

        if (opened)
        {
            // Placeholder for children
            ImGui::TreePop();
        }

        if (entityDeleted)
        {
            context_->DestroyEntity(entity);
            if (selection_context_ == entity)
                selection_context_ = ecs::Entity();
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

        ImGui::SameLine();
        ImGui::PushItemWidth(-1);

        if (ImGui::Button("Add Component"))
            ImGui::OpenPopup("AddComponent");

        if (ImGui::BeginPopup("AddComponent"))
        {
            if (ImGui::MenuItem("Sprite Component"))
            {
                if (!context_->HasComponent<ecs::SpriteComponent>(entity))
                    context_->AddComponent(entity, ecs::SpriteComponent{});
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::MenuItem("Native Script Component"))
            {
                if (!context_->HasComponent<ecs::NativeScriptComponent>(entity))
                    context_->AddComponent(entity, ecs::NativeScriptComponent{});
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        ImGui::PopItemWidth();

        ImGui::Separator();

        if (context_->HasComponent<ecs::TransformComponent>(entity))
        {
            if (ImGui::TreeNodeEx((void*)typeid(ecs::TransformComponent).hash_code(), ImGuiTreeNodeFlags_DefaultOpen, "Transform"))
            {
                auto& tc = context_->GetComponent<ecs::TransformComponent>(entity);
                ImGui::DragFloat3("Position", &tc.position.x, 0.1f);
                
                // Rotation (simple quat-to-euler representation for display)
                static Math::Vec3f rotation = { 0, 0, 0 }; 
                if (ImGui::DragFloat3("Rotation", &rotation.x, 0.1f))
                {
                    tc.rotation = Math::Quatf::FromEuler(rotation);
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
                // Texture control (to be added)
                ImGui::TreePop();
            }
        }

        if (context_->HasComponent<ecs::NativeScriptComponent>(entity))
        {
            if (ImGui::TreeNodeEx((void*)typeid(ecs::NativeScriptComponent).hash_code(), ImGuiTreeNodeFlags_DefaultOpen, "Native Script"))
            {
                auto& nsc = context_->GetComponent<ecs::NativeScriptComponent>(entity);
                ImGui::Text("Script Attached");
                if (ImGui::Button("Remove Script"))
                {
                    context_->RemoveComponent<ecs::NativeScriptComponent>(entity);
                }
                ImGui::TreePop();
            }
        }
    }

} // namespace editor
} // namespace ge
