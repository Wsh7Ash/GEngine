#include "SceneSerializer.h"
#include "../debug/log.h"
#include "../ecs/components/MeshComponent.h"
#include "../ecs/components/NativeScriptComponent.h"
#include "../ecs/components/SpriteComponent.h"
#include "../ecs/components/TransformComponent.h"
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace ge {
namespace scene {

SceneSerializer::SceneSerializer(ecs::World &world) : world_(world) {}

void SceneSerializer::Serialize(const std::string &filepath) {
  json root;
  root["Scene"] = "Untitled";
  root["Entities"] = json::array();

  // Iterate through all entities (rough implementation for now)
  // In a real ECS, we'd iterate through a specific view or registry
  for (uint32_t i = 0; i < 10000; ++i) {
    ecs::Entity entity(i); // Explicitly construct handle

    // Check if entity is "alive" using a simple heuristic (e.g., has Transform)
    if (!world_.HasComponent<ecs::TransformComponent>(entity))
      continue;

    json entityJson;
    entityJson["ID"] = i;

    // Serialize Transform
    auto &tc = world_.GetComponent<ecs::TransformComponent>(entity);
    entityJson["Transform"] = {
        {"Translation", {tc.position.x, tc.position.y, tc.position.z}},
        {"Rotation",
         {tc.rotation.w, tc.rotation.x, tc.rotation.y, tc.rotation.z}},
        {"Scale", {tc.scale.x, tc.scale.y, tc.scale.z}}};

    // Serialize Sprite (if present)
    if (world_.HasComponent<ecs::SpriteComponent>(entity)) {
      auto &sc = world_.GetComponent<ecs::SpriteComponent>(entity);
      entityJson["Sprite"] = {
          {"Color", {sc.color.x, sc.color.y, sc.color.z, sc.color.w}}};
    }

    // Serialize NativeScript (if present)
    if (world_.HasComponent<ecs::NativeScriptComponent>(entity)) {
      auto &nsc = world_.GetComponent<ecs::NativeScriptComponent>(entity);
      json scriptJson;
      if (nsc.instance) {
        nsc.instance->OnSerialize(&scriptJson);
      }
      entityJson["NativeScript"] = scriptJson;
    }

    root["Entities"].push_back(entityJson);
  }

  std::ofstream fout(filepath);
  if (fout.is_open()) {
    fout << root.dump(4);
    GE_LOG_INFO("Scene serialized to %s", filepath.c_str());
  }
}

bool SceneSerializer::Deserialize(const std::string &filepath) {
  std::ifstream fin(filepath);
  if (!fin.is_open())
    return false;

  json data;
  try {
    fin >> data;
  } catch (json::parse_error &e) {
    GE_LOG_ERROR("Failed to parse scene JSON: %s", e.what());
    return false;
  }

  if (!data.contains("Entities"))
    return false;

  for (auto &entityData : data["Entities"]) {
    // uint32_t id = entityData["ID"]; // Original ID not strictly used for
    // restoration yet
    ecs::Entity entity = world_.CreateEntity();

    // Deserialize Transform
    if (entityData.contains("Transform")) {
      auto &tData = entityData["Transform"]["Translation"];
      auto &rData = entityData["Transform"]["Rotation"];
      auto &sData = entityData["Transform"]["Scale"];

      ecs::TransformComponent tc;
      tc.position = {(float)tData[0], (float)tData[1], (float)tData[2]};
      tc.rotation = {(float)rData[0], (float)rData[1], (float)rData[2],
                     (float)rData[3]};
      tc.scale = {(float)sData[0], (float)sData[1], (float)sData[2]};
      world_.AddComponent(entity, tc);
    }

    // Deserialize Sprite
    if (entityData.contains("Sprite")) {
      auto &cData = entityData["Sprite"]["Color"];
      ecs::SpriteComponent sc;
      sc.color = {(float)cData[0], (float)cData[1], (float)cData[2],
                  (float)cData[3]};
      world_.AddComponent(entity, sc);
    }

    // Deserialize NativeScript
    if (entityData.contains("NativeScript")) {
      // Note: We can only deserialize if the script is already bound
      if (world_.HasComponent<ecs::NativeScriptComponent>(entity)) {
        auto &nsc = world_.GetComponent<ecs::NativeScriptComponent>(entity);
        if (nsc.instance) {
          nsc.instance->OnDeserialize(&entityData["NativeScript"]);
        }
      }
    }
  }

  GE_LOG_INFO("Scene deserialized from %s", filepath.c_str());
  return true;
}

} // namespace scene
} // namespace ge
