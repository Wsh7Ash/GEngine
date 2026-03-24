#include "SceneSerializer.h"
#include "../ecs/components/IDComponent.h"
#include "../ecs/components/TagComponent.h"
#include "../ecs/components/TransformComponent.h"
#include "../ecs/components/MeshComponent.h"
#include "../ecs/components/NativeScriptComponent.h"
#include "../ecs/components/SpriteComponent.h"
#include "../ecs/components/Rigidbody2DComponent.h"
#include "../ecs/components/BoxCollider2DComponent.h"
#include "../ecs/components/RelationshipComponent.h"
#include "../debug/log.h"
#include "../ecs/ScriptableEntity.h"
#include "../ecs/ScriptRegistry.h"
#include <fstream>
#include <nlohmann/json.hpp>

namespace ge {
namespace scene {

using json = nlohmann::json;

SceneSerializer::SceneSerializer(ecs::World &world) : world_(world) {}

bool SceneSerializer::Serialize(const std::string &filepath) {
  json root;
  root["Scene"] = "Untitled";
  root["Entities"] = json::array();

  try {
    // Collect all unique entities from the world
      for (auto const &entity : world_.Query<::ge::ecs::TagComponent>()) {
      json entityJson;
      
      // Serialize UUID (Stable ID)
      if (world_.HasComponent<::ge::ecs::IDComponent>(entity)) {
        entityJson["UUID"] = (uint64_t)world_.GetComponent<::ge::ecs::IDComponent>(entity).ID;
      }

      // Serialize Tag
      if (world_.HasComponent<::ge::ecs::TagComponent>(entity)) {
        entityJson["Tag"] = world_.GetComponent<::ge::ecs::TagComponent>(entity).tag;
      }

      // Serialize Transform
      if (world_.HasComponent<ecs::TransformComponent>(entity)) {
        auto &tc = world_.GetComponent<ecs::TransformComponent>(entity);
        entityJson["Transform"] = {
            {"Translation", {tc.position.x, tc.position.y, tc.position.z}},
            {"Rotation",
             {tc.rotation.w, tc.rotation.x, tc.rotation.y, tc.rotation.z}},
            {"Scale", {tc.scale.x, tc.scale.y, tc.scale.z}}};
      }

      // Serialize Mesh
      if (world_.HasComponent<ecs::MeshComponent>(entity)) {
        auto &mc = world_.GetComponent<ecs::MeshComponent>(entity);
        entityJson["Mesh"] = {{"MeshPath", mc.MeshPath},
                              {"ShaderPath", mc.ShaderPath}};
      }

      // Serialize Sprite
      if (world_.HasComponent<ecs::SpriteComponent>(entity)) {
        auto &sc = world_.GetComponent<ecs::SpriteComponent>(entity);
        entityJson["Sprite"] = {
            {"Color", {sc.color.x, sc.color.y, sc.color.z, sc.color.w}},
            {"FlipX", sc.FlipX},
            {"FlipY", sc.FlipY},
            {"isAnimated", sc.isAnimated},
            {"framesX", sc.framesX},
            {"framesY", sc.framesY},
            {"frameTime", sc.frameTime}};
      }

      // Serialize NativeScript
      if (world_.HasComponent<ecs::NativeScriptComponent>(entity)) {
        auto &nsc = world_.GetComponent<ecs::NativeScriptComponent>(entity);
        json nscJson;
        nscJson["Name"] = nsc.ScriptName;
        if (nsc.instance) {
          json scriptData;
          nsc.instance->OnSerialize(&scriptData);
          nscJson["Data"] = scriptData;
        }
        entityJson["NativeScript"] = nscJson;
      }

      // Serialize Rigidbody2D
      if (world_.HasComponent<ecs::Rigidbody2DComponent>(entity)) {
        auto &rb = world_.GetComponent<ecs::Rigidbody2DComponent>(entity);
        entityJson["Rigidbody2D"] = {
            {"Type", (int)rb.Type},
            {"FixedRotation", rb.FixedRotation}};
      }

      // Serialize BoxCollider2D
      if (world_.HasComponent<ecs::BoxCollider2DComponent>(entity)) {
        auto &bc = world_.GetComponent<ecs::BoxCollider2DComponent>(entity);
        entityJson["BoxCollider2D"] = {
            {"Offset", {bc.Offset.x, bc.Offset.y}},
            {"Size", {bc.Size.x, bc.Size.y}},
            {"Density", bc.Density},
            {"Friction", bc.Friction},
            {"Restitution", bc.Restitution}};
      }

      // Serialize Hierarchy
      if (world_.HasComponent<ecs::RelationshipComponent>(entity)) {
        auto &rc = world_.GetComponent<ecs::RelationshipComponent>(entity);
        if (rc.Parent != ecs::INVALID_ENTITY && world_.HasComponent<ecs::IDComponent>(rc.Parent)) {
          entityJson["Parent"] = (uint64_t)world_.GetComponent<ecs::IDComponent>(rc.Parent).ID;
        }
      }

      root["Entities"].push_back(entityJson);
    }

    std::ofstream fout(filepath);
    if (!fout.is_open()) {
      GE_LOG_ERROR("Could not open file for scene serialization: %s",
                   filepath.c_str());
      return false;
    }

    fout << root.dump(4);
    GE_LOG_INFO("Scene serialized to %s", filepath.c_str());
    return true;
  } catch (const nlohmann::json::exception &e) {
    GE_LOG_ERROR("Scene serialization failed (JSON error): %s. Check for "
                 "NaN/Inf values in components.",
                 e.what());
    return false;
  } catch (const std::exception &e) {
    GE_LOG_ERROR("Scene serialization failed: %s", e.what());
    return false;
  }
}

bool SceneSerializer::Deserialize(const std::string &filepath) {
  std::ifstream fin(filepath);
  if (!fin.is_open())
    return false;

  json data;
  try {
    fin >> data;
  } catch (const nlohmann::json::exception &e) {
    GE_LOG_ERROR("Scene deserialization failed: %s", e.what());
    return false;
  } catch (const std::exception &e) {
    GE_LOG_ERROR("An unexpected error occurred during deserialization: %s",
                 e.what());
    return false;
  }

  if (!data.contains("Entities"))
    return false;

  for (auto &entityData : data["Entities"]) {
    ecs::Entity entity;
    if (entityData.contains("UUID")) {
      uint64_t uuid = entityData["UUID"];
      entity = world_.CreateEntityWithUUID(uuid);
    } else {
      entity = world_.CreateEntity();
    }

    // Deserialize Tag
    if (entityData.contains("Tag")) {
      world_.AddComponent(entity, ::ge::ecs::TagComponent{entityData["Tag"]});
    }

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

    // Deserialize Mesh
    if (entityData.contains("Mesh")) {
      ecs::MeshComponent mc;
      mc.MeshPath = entityData["Mesh"]["MeshPath"];
      mc.ShaderPath = entityData["Mesh"]["ShaderPath"];
      // Note: Actual asset loading from paths would happen here if implemented
      world_.AddComponent(entity, mc);
    }

    // Deserialize Sprite
    if (entityData.contains("Sprite")) {
      auto &cData = entityData["Sprite"]["Color"];
      ecs::SpriteComponent sc;
      sc.color = {(float)cData[0], (float)cData[1], (float)cData[2],
                  (float)cData[3]};
      if (entityData["Sprite"].contains("FlipX"))
        sc.FlipX = entityData["Sprite"]["FlipX"];
      if (entityData["Sprite"].contains("FlipY"))
        sc.FlipY = entityData["Sprite"]["FlipY"];
      if (entityData["Sprite"].contains("isAnimated"))
        sc.isAnimated = entityData["Sprite"]["isAnimated"];
      if (entityData["Sprite"].contains("framesX"))
        sc.framesX = entityData["Sprite"]["framesX"];
      if (entityData["Sprite"].contains("framesY"))
        sc.framesY = entityData["Sprite"]["framesY"];
      if (entityData["Sprite"].contains("frameTime"))
        sc.frameTime = entityData["Sprite"]["frameTime"];

      world_.AddComponent(entity, sc);
    }

    // Deserialize NativeScript
    if (entityData.contains("NativeScript")) {
      std::string scriptName = entityData["NativeScript"]["Name"];
      ecs::NativeScriptComponent nsc;
      ecs::ScriptRegistry::BindByName(&nsc, scriptName);
      world_.AddComponent(entity, std::move(nsc));

      if (entityData["NativeScript"].contains("Data")) {
        auto &targetNsc = world_.GetComponent<ecs::NativeScriptComponent>(entity);
        if (targetNsc.instance) {
          targetNsc.instance->OnDeserialize(&entityData["NativeScript"]["Data"]);
        }
      }
    }

    // Deserialize Rigidbody2D
    if (entityData.contains("Rigidbody2D")) {
      ecs::Rigidbody2DComponent rb;
      rb.Type = (ecs::RigidBody2DType)entityData["Rigidbody2D"]["Type"];
      rb.FixedRotation = entityData["Rigidbody2D"]["FixedRotation"];
      world_.AddComponent(entity, rb);
    }

    // Deserialize BoxCollider2D
    if (entityData.contains("BoxCollider2D")) {
      ecs::BoxCollider2DComponent bc;
      auto &oData = entityData["BoxCollider2D"]["Offset"];
      auto &sData = entityData["BoxCollider2D"]["Size"];
      bc.Offset = {(float)oData[0], (float)oData[1]};
      bc.Size = {(float)sData[0], (float)sData[1]};
      bc.Density = entityData["BoxCollider2D"]["Density"];
      bc.Friction = entityData["BoxCollider2D"]["Friction"];
      bc.Restitution = entityData["BoxCollider2D"]["Restitution"];
      world_.AddComponent(entity, bc);
    }

    // Deserialize Parent (by UUID)
    if (entityData.contains("Parent")) {
      UUID parentUUID = entityData["Parent"].get<uint64_t>();
      ecs::Entity parent = world_.GetEntityByUUID(parentUUID);
      if (parent != ecs::INVALID_ENTITY) {
        world_.SetParent(entity, parent);
      }
    }
  }

  GE_LOG_INFO("Scene deserialized from %s", filepath.c_str());
  return true;
}

} // namespace scene
} // namespace ge
