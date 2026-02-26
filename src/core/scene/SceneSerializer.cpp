#include "SceneSerializer.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include "../ecs/components/TransformComponent.h"
#include "../ecs/components/SpriteComponent.h"
#include "../ecs/components/MeshComponent.h"
#include "../debug/log.h"

using json = nlohmann::json;

namespace ge {
namespace scene {

    SceneSerializer::SceneSerializer(ecs::World& world)
        : world_(world) { }

    void SceneSerializer::Serialize(const std::string& filepath)
    {
        json root;
        root["Scene"] = "Untitled";
        root["Entities"] = json::array();

        // Iterate through all entities (rough implementation for now)
        // In a real ECS, we'd iterate through a specific view or registry
        for (ecs::Entity entity = 0; entity < 10000; ++entity) // Using a safe upper bound for demo
        {
            // Check if entity is "alive" using a simple heuristic (e.g., has Transform)
            // Ideally World would provide an ActiveEntities list
            if (!world_.HasComponent<ecs::TransformComponent>(entity))
                continue;

            json entityJson;
            entityJson["ID"] = entity;

            // Serialize Transform
            auto& tc = world_.GetComponent<ecs::TransformComponent>(entity);
            entityJson["Transform"] = {
                {"Translation", {tc.position.x, tc.position.y, tc.position.z}},
                {"Rotation", {tc.rotation.w, tc.rotation.x, tc.rotation.y, tc.rotation.z}},
                {"Scale", {tc.scale.x, tc.scale.y, tc.scale.z}}
            };

            // Serialize Sprite (if present)
            if (world_.HasComponent<ecs::SpriteComponent>(entity))
            {
                auto& sc = world_.GetComponent<ecs::SpriteComponent>(entity);
                entityJson["Sprite"] = {
                    {"Color", {sc.color.r, sc.color.g, sc.color.b, sc.color.a}}
                    // Texture serialization involves asset paths, to be added later
                };
            }

            root["Entities"].push_back(entityJson);
        }

        std::ofstream fout(filepath);
        fout << root.dump(4);
        GE_LOG_INFO("Scene serialized to %s", filepath.c_str());
    }

    bool SceneSerializer::Deserialize(const std::string& filepath)
    {
        std::ifstream fin(filepath);
        if (!fin.is_open()) return false;

        json data;
        fin >> data;

        if (!data.contains("Entities")) return false;

        for (auto& entityData : data["Entities"])
        {
            uint32_t id = entityData["ID"];
            ecs::Entity entity = world_.CreateEntity(); // Should probably reserve specific IDs

            // Deserialize Transform
            if (entityData.contains("Transform"))
            {
                auto& tData = entityData["Transform"]["Translation"];
                auto& rData = entityData["Transform"]["Rotation"];
                auto& sData = entityData["Transform"]["Scale"];

                ecs::TransformComponent tc;
                tc.position = { tData[0], tData[1], tData[2] };
                tc.rotation = { rData[0], rData[1], rData[2], rData[3] };
                tc.scale = { sData[0], sData[1], sData[2] };
                world_.AddComponent(entity, tc);
            }

            // Deserialize Sprite
            if (entityData.contains("Sprite"))
            {
                auto& cData = entityData["Sprite"]["Color"];
                ecs::SpriteComponent sc;
                sc.color = { cData[0], cData[1], cData[2], cData[3] };
                world_.AddComponent(entity, sc);
            }
        }

        GE_LOG_INFO("Scene deserialized from %s", filepath.c_str());
        return true;
    }

} // namespace scene
} // namespace ge
