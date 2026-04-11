#include "SceneSerializer.h"
#include "../ecs/components/IDComponent.h"
#include "../ecs/components/TagComponent.h"
#include "../ecs/components/TransformComponent.h"
#include "../ecs/components/MeshComponent.h"
#include "../ecs/components/LightComponent.h"
#include "../ecs/components/NativeScriptComponent.h"
#include "../ecs/components/SpriteComponent.h"
#include "../ecs/components/TilemapComponent.h"
#include "../ecs/components/GridPathData.h"
#include "../ecs/components/TopDownControllerComponent.h"
#include "../ecs/components/InteractionComponent.h"
#include "../ecs/components/HealthComponent.h"
#include "../ecs/components/InventoryComponent.h"
#include "../ecs/components/PickupComponent.h"
#include "../ecs/components/ResourceNodeComponent.h"
#include "../ecs/components/BuildPlacementComponent.h"
#include "../ecs/components/WaveSpawnerComponent.h"
#include "../ecs/components/DefenseTowerComponent.h"
#include "../ecs/components/Rigidbody2DComponent.h"
#include "../ecs/components/BoxCollider2DComponent.h"
#include "../ecs/components/RelationshipComponent.h"
#include "../debug/log.h"
#include "../ecs/ScriptableEntity.h"
#include "../ecs/ScriptRegistry.h"
#include "../renderer/Texture.h"
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
        entityJson["Mesh"] = {
            {"MeshPath", mc.MeshPath},
            {"AlbedoColor", {mc.AlbedoColor.x, mc.AlbedoColor.y, mc.AlbedoColor.z}},
            {"Metallic", mc.Metallic},
            {"Roughness", mc.Roughness},
            {"AlbedoPath", mc.AlbedoPath},
            {"NormalPath", mc.NormalPath},
            {"MetallicPath", mc.MetallicPath},
            {"RoughnessPath", mc.RoughnessPath},
            {"AOPath", mc.AOPath}
        };
      }

      // Serialize Light
      if (world_.HasComponent<ecs::LightComponent>(entity)) {
        auto &lc = world_.GetComponent<ecs::LightComponent>(entity);
        entityJson["Light"] = {
            {"Type", (int)lc.Type},
            {"Color", {lc.Color.x, lc.Color.y, lc.Color.z}},
            {"Intensity", lc.Intensity},
            {"Range", lc.Range},
            {"CastShadows", lc.CastShadows}
        };
      }

      // Serialize Sprite
      if (world_.HasComponent<ecs::SpriteComponent>(entity)) {
        auto &sc = world_.GetComponent<ecs::SpriteComponent>(entity);
        entityJson["Sprite"] = {
            {"TexturePath", sc.TexturePath},
            {"Color", {sc.color.x, sc.color.y, sc.color.z, sc.color.w}},
            {"Pivot", {sc.Pivot.x, sc.Pivot.y}},
            {"FlipX", sc.FlipX},
            {"FlipY", sc.FlipY},
            {"SortingLayer", sc.SortingLayer},
            {"OrderInLayer", sc.OrderInLayer},
            {"YSort", sc.YSort},
            {"PixelsPerUnit", sc.PixelsPerUnit},
            {"UseSourceSize", sc.UseSourceSize},
            {"UseAtlasRegion", sc.UseAtlasRegion},
            {"AtlasRegion", {sc.AtlasRegion.x, sc.AtlasRegion.y, sc.AtlasRegion.z, sc.AtlasRegion.w}},
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

      if (world_.HasComponent<ecs::TilemapComponent>(entity)) {
        auto& tilemap = world_.GetComponent<ecs::TilemapComponent>(entity);
        json layers = json::array();
        for (const auto& layer : tilemap.Layers) {
          layers.push_back({
            {"Name", layer.Name},
            {"Tiles", layer.Tiles},
            {"Visible", layer.Visible},
            {"CollisionLayer", layer.CollisionLayer},
            {"ZOffset", layer.ZOffset}
          });
        }

        json palette = json::array();
        for (const auto& color : tilemap.TilePalette) {
          palette.push_back({color.x, color.y, color.z, color.w});
        }

        entityJson["Tilemap"] = {
          {"TilesetTexturePath", tilemap.TilesetTexturePath},
          {"Width", tilemap.Width},
          {"Height", tilemap.Height},
          {"TileWidth", tilemap.TileWidth},
          {"TileHeight", tilemap.TileHeight},
          {"TilesPerRow", tilemap.TilesPerRow},
          {"PixelsPerUnit", tilemap.PixelsPerUnit},
          {"ChunkWidth", tilemap.ChunkWidth},
          {"ChunkHeight", tilemap.ChunkHeight},
          {"AutoBuildNavigation", tilemap.AutoBuildNavigation},
          {"NavigationOrigin", {tilemap.Navigation.Origin.x, tilemap.Navigation.Origin.y}},
          {"NavigationCellSize", tilemap.Navigation.CellSize},
          {"TilePalette", palette},
          {"Layers", layers}
        };
      }

      if (world_.HasComponent<ecs::GridMapComponent>(entity)) {
        auto& grid = world_.GetComponent<ecs::GridMapComponent>(entity).Grid;
        entityJson["GridMap"] = {
          {"Width", grid.Width},
          {"Height", grid.Height},
          {"CellSize", grid.CellSize},
          {"Origin", {grid.Origin.x, grid.Origin.y}},
          {"Blocked", grid.Blocked}
        };
      }

      if (world_.HasComponent<ecs::TopDownControllerComponent>(entity)) {
        auto& controller = world_.GetComponent<ecs::TopDownControllerComponent>(entity);
        entityJson["TopDownController"] = {{"MoveSpeed", controller.MoveSpeed}};
      }

      if (world_.HasComponent<ecs::InteractionComponent>(entity)) {
        auto& interaction = world_.GetComponent<ecs::InteractionComponent>(entity);
        entityJson["Interaction"] = {
          {"Range", interaction.Range},
          {"Enabled", interaction.Enabled},
          {"Prompt", interaction.Prompt}
        };
      }

      if (world_.HasComponent<ecs::HealthComponent>(entity)) {
        auto& health = world_.GetComponent<ecs::HealthComponent>(entity);
        entityJson["Health"] = {
          {"Current", health.Current},
          {"Max", health.Max},
          {"DestroyOnZero", health.DestroyOnZero}
        };
      }

      if (world_.HasComponent<ecs::InventoryComponent>(entity)) {
        auto& inventory = world_.GetComponent<ecs::InventoryComponent>(entity);
        json items = json::array();
        for (const auto& stack : inventory.Items) {
          items.push_back({{"ItemId", stack.ItemId}, {"Quantity", stack.Quantity}});
        }
        entityJson["Inventory"] = {
          {"Capacity", inventory.Capacity},
          {"Items", items}
        };
      }

      if (world_.HasComponent<ecs::PickupComponent>(entity)) {
        auto& pickup = world_.GetComponent<ecs::PickupComponent>(entity);
        entityJson["Pickup"] = {
          {"ItemId", pickup.ItemId},
          {"Quantity", pickup.Quantity},
          {"AutoPickupRadius", pickup.AutoPickupRadius}
        };
      }

      if (world_.HasComponent<ecs::ResourceNodeComponent>(entity)) {
        auto& resource = world_.GetComponent<ecs::ResourceNodeComponent>(entity);
        entityJson["ResourceNode"] = {
          {"ItemId", resource.ItemId},
          {"Amount", resource.Amount},
          {"MaxAmount", resource.MaxAmount},
          {"YieldPerInteract", resource.YieldPerInteract},
          {"RespawnDelay", resource.RespawnDelay},
          {"RespawnTimer", resource.RespawnTimer},
          {"Depleted", resource.Depleted}
        };
      }

      if (world_.HasComponent<ecs::BuildPlacementComponent>(entity)) {
        auto& placement = world_.GetComponent<ecs::BuildPlacementComponent>(entity);
        entityJson["BuildPlacement"] = {
          {"BlocksPath", placement.BlocksPath},
          {"DisallowBlockingPath", placement.DisallowBlockingPath},
          {"PlacementCooldown", placement.PlacementCooldown},
          {"PlacementTimer", placement.PlacementTimer},
          {"RemainingPlacements", placement.RemainingPlacements},
          {"DefenseColor", {placement.DefenseColor.x, placement.DefenseColor.y, placement.DefenseColor.z, placement.DefenseColor.w}}
        };
      }

      if (world_.HasComponent<ecs::WaveSpawnerComponent>(entity)) {
        auto& spawner = world_.GetComponent<ecs::WaveSpawnerComponent>(entity);
        entityJson["WaveSpawner"] = {
          {"SpawnCell", {spawner.SpawnCell.x, spawner.SpawnCell.y}},
          {"GoalCell", {spawner.GoalCell.x, spawner.GoalCell.y}},
          {"SpawnInterval", spawner.SpawnInterval},
          {"SpawnTimer", spawner.SpawnTimer},
          {"WaveInterval", spawner.WaveInterval},
          {"WaveTimer", spawner.WaveTimer},
          {"EnemySpeed", spawner.EnemySpeed},
          {"EnemyHealth", spawner.EnemyHealth},
          {"EnemiesPerWave", spawner.EnemiesPerWave},
          {"RemainingInWave", spawner.RemainingInWave},
          {"LoopWaves", spawner.LoopWaves},
          {"EnemyColor", {spawner.EnemyColor.x, spawner.EnemyColor.y, spawner.EnemyColor.z, spawner.EnemyColor.w}}
        };
      }

      if (world_.HasComponent<ecs::DefenseTowerComponent>(entity)) {
        auto& tower = world_.GetComponent<ecs::DefenseTowerComponent>(entity);
        entityJson["DefenseTower"] = {
          {"Range", tower.Range},
          {"FireInterval", tower.FireInterval},
          {"Cooldown", tower.Cooldown},
          {"Damage", tower.Damage}
        };
      }

      if (world_.HasComponent<ecs::PathAgentComponent>(entity)) {
        auto& agent = world_.GetComponent<ecs::PathAgentComponent>(entity);
        json path = json::array();
        for (const auto& cell : agent.Path) {
          path.push_back({cell.x, cell.y});
        }
        entityJson["PathAgent"] = {
          {"GoalCell", {agent.GoalCell.x, agent.GoalCell.y}},
          {"MoveSpeed", agent.MoveSpeed},
          {"NextWaypointIndex", agent.NextWaypointIndex},
          {"DestroyAtGoal", agent.DestroyAtGoal},
          {"RepathRequired", agent.RepathRequired},
          {"Path", path}
        };
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

  world_.Clear();
  std::vector<std::pair<ecs::Entity, UUID>> pendingParents;

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
      
      if (entityData["Mesh"].contains("AlbedoColor")) {
        auto &aData = entityData["Mesh"]["AlbedoColor"];
        mc.AlbedoColor = {(float)aData[0], (float)aData[1], (float)aData[2]};
      }
      if (entityData["Mesh"].contains("Metallic")) mc.Metallic = entityData["Mesh"]["Metallic"];
      if (entityData["Mesh"].contains("Roughness")) mc.Roughness = entityData["Mesh"]["Roughness"];
      
      if (entityData["Mesh"].contains("AlbedoPath")) mc.AlbedoPath = entityData["Mesh"]["AlbedoPath"];
      if (entityData["Mesh"].contains("NormalPath")) mc.NormalPath = entityData["Mesh"]["NormalPath"];
      if (entityData["Mesh"].contains("MetallicPath")) mc.MetallicPath = entityData["Mesh"]["MetallicPath"];
      if (entityData["Mesh"].contains("RoughnessPath")) mc.RoughnessPath = entityData["Mesh"]["RoughnessPath"];
      if (entityData["Mesh"].contains("AOPath")) mc.AOPath = entityData["Mesh"]["AOPath"];

      world_.AddComponent(entity, mc);
    }

    // Deserialize Light
    if (entityData.contains("Light")) {
      ecs::LightComponent lc;
      lc.Type = (ecs::LightType)entityData["Light"]["Type"];
      auto &cData = entityData["Light"]["Color"];
      lc.Color = {(float)cData[0], (float)cData[1], (float)cData[2]};
      lc.Intensity = entityData["Light"]["Intensity"];
      lc.Range = entityData["Light"]["Range"];
      lc.CastShadows = entityData["Light"]["CastShadows"];
      world_.AddComponent(entity, lc);
    }

    // Deserialize Sprite
    if (entityData.contains("Sprite")) {
      auto &cData = entityData["Sprite"]["Color"];
      ecs::SpriteComponent sc;
      if (entityData["Sprite"].contains("TexturePath")) {
        sc.TexturePath = entityData["Sprite"]["TexturePath"];
        if (!sc.TexturePath.empty()) {
          renderer::TextureSpecification textureSpec;
          textureSpec.PixelArt = true;
          sc.texture = renderer::Texture::Create(sc.TexturePath, textureSpec);
        }
      }
      sc.color = {(float)cData[0], (float)cData[1], (float)cData[2],
                  (float)cData[3]};
      if (entityData["Sprite"].contains("Pivot")) {
        auto& pData = entityData["Sprite"]["Pivot"];
        sc.Pivot = {(float)pData[0], (float)pData[1]};
      }
      if (entityData["Sprite"].contains("FlipX"))
        sc.FlipX = entityData["Sprite"]["FlipX"];
      if (entityData["Sprite"].contains("FlipY"))
        sc.FlipY = entityData["Sprite"]["FlipY"];
      if (entityData["Sprite"].contains("SortingLayer"))
        sc.SortingLayer = entityData["Sprite"]["SortingLayer"];
      if (entityData["Sprite"].contains("OrderInLayer"))
        sc.OrderInLayer = entityData["Sprite"]["OrderInLayer"];
      if (entityData["Sprite"].contains("YSort"))
        sc.YSort = entityData["Sprite"]["YSort"];
      if (entityData["Sprite"].contains("PixelsPerUnit"))
        sc.PixelsPerUnit = entityData["Sprite"]["PixelsPerUnit"];
      if (entityData["Sprite"].contains("UseSourceSize"))
        sc.UseSourceSize = entityData["Sprite"]["UseSourceSize"];
      if (entityData["Sprite"].contains("UseAtlasRegion"))
        sc.UseAtlasRegion = entityData["Sprite"]["UseAtlasRegion"];
      if (entityData["Sprite"].contains("AtlasRegion")) {
        auto& aData = entityData["Sprite"]["AtlasRegion"];
        sc.AtlasRegion = {(float)aData[0], (float)aData[1], (float)aData[2], (float)aData[3]};
      }
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

    if (entityData.contains("Tilemap")) {
      ecs::TilemapComponent tilemap;
      auto& tilemapData = entityData["Tilemap"];
      if (tilemapData.contains("TilesetTexturePath")) {
        tilemap.TilesetTexturePath = tilemapData["TilesetTexturePath"];
        if (!tilemap.TilesetTexturePath.empty()) {
          renderer::TextureSpecification textureSpec;
          textureSpec.PixelArt = true;
          tilemap.TilesetTexture = renderer::Texture::Create(tilemap.TilesetTexturePath, textureSpec);
        }
      }
      tilemap.Width = tilemapData.value("Width", 0);
      tilemap.Height = tilemapData.value("Height", 0);
      tilemap.TileWidth = tilemapData.value("TileWidth", 16);
      tilemap.TileHeight = tilemapData.value("TileHeight", 16);
      tilemap.TilesPerRow = tilemapData.value("TilesPerRow", 1);
      tilemap.PixelsPerUnit = tilemapData.value("PixelsPerUnit", 16.0f);
      tilemap.ChunkWidth = tilemapData.value("ChunkWidth", 16);
      tilemap.ChunkHeight = tilemapData.value("ChunkHeight", 16);
      tilemap.AutoBuildNavigation = tilemapData.value("AutoBuildNavigation", true);
      if (tilemapData.contains("NavigationOrigin")) {
        auto& origin = tilemapData["NavigationOrigin"];
        tilemap.Navigation.Origin = {(float)origin[0], (float)origin[1]};
      }
      tilemap.Navigation.CellSize = tilemapData.value("NavigationCellSize", 1.0f);
      tilemap.Navigation.Resize(tilemap.Width, tilemap.Height, false);
      if (tilemapData.contains("TilePalette")) {
        for (auto& color : tilemapData["TilePalette"]) {
          tilemap.TilePalette.push_back({(float)color[0], (float)color[1], (float)color[2], (float)color[3]});
        }
      }
      if (tilemapData.contains("Layers")) {
        for (auto& layerData : tilemapData["Layers"]) {
          ecs::TilemapLayer layer;
          layer.Name = layerData.value("Name", "Layer");
          layer.Visible = layerData.value("Visible", true);
          layer.CollisionLayer = layerData.value("CollisionLayer", false);
          layer.ZOffset = layerData.value("ZOffset", 0.0f);
          if (layerData.contains("Tiles")) {
            layer.Tiles = layerData["Tiles"].get<std::vector<int32_t>>();
          }
          tilemap.Layers.push_back(std::move(layer));
        }
      }
      world_.AddComponent(entity, std::move(tilemap));
    }

    if (entityData.contains("GridMap")) {
      ecs::GridMapComponent gridMap;
      auto& gridData = entityData["GridMap"];
      gridMap.Grid.Resize(gridData.value("Width", 0), gridData.value("Height", 0), false);
      gridMap.Grid.CellSize = gridData.value("CellSize", 1.0f);
      if (gridData.contains("Origin")) {
        auto& origin = gridData["Origin"];
        gridMap.Grid.Origin = {(float)origin[0], (float)origin[1]};
      }
      if (gridData.contains("Blocked")) {
        gridMap.Grid.Blocked = gridData["Blocked"].get<std::vector<uint8_t>>();
      }
      world_.AddComponent(entity, std::move(gridMap));
    }

    if (entityData.contains("TopDownController")) {
      ecs::TopDownControllerComponent controller;
      controller.MoveSpeed = entityData["TopDownController"].value("MoveSpeed", controller.MoveSpeed);
      world_.AddComponent(entity, controller);
    }

    if (entityData.contains("Interaction")) {
      ecs::InteractionComponent interaction;
      interaction.Range = entityData["Interaction"].value("Range", interaction.Range);
      interaction.Enabled = entityData["Interaction"].value("Enabled", interaction.Enabled);
      interaction.Prompt = entityData["Interaction"].value("Prompt", interaction.Prompt);
      world_.AddComponent(entity, std::move(interaction));
    }

    if (entityData.contains("Health")) {
      ecs::HealthComponent health;
      health.Current = entityData["Health"].value("Current", health.Current);
      health.Max = entityData["Health"].value("Max", health.Max);
      health.DestroyOnZero = entityData["Health"].value("DestroyOnZero", health.DestroyOnZero);
      world_.AddComponent(entity, health);
    }

    if (entityData.contains("Inventory")) {
      ecs::InventoryComponent inventory;
      inventory.Capacity = entityData["Inventory"].value("Capacity", inventory.Capacity);
      if (entityData["Inventory"].contains("Items")) {
        for (auto& itemData : entityData["Inventory"]["Items"]) {
          inventory.Items.push_back({
            itemData.value("ItemId", std::string{}),
            itemData.value("Quantity", 0)
          });
        }
      }
      world_.AddComponent(entity, std::move(inventory));
    }

    if (entityData.contains("Pickup")) {
      ecs::PickupComponent pickup;
      pickup.ItemId = entityData["Pickup"].value("ItemId", pickup.ItemId);
      pickup.Quantity = entityData["Pickup"].value("Quantity", pickup.Quantity);
      pickup.AutoPickupRadius = entityData["Pickup"].value("AutoPickupRadius", pickup.AutoPickupRadius);
      world_.AddComponent(entity, std::move(pickup));
    }

    if (entityData.contains("ResourceNode")) {
      ecs::ResourceNodeComponent resource;
      resource.ItemId = entityData["ResourceNode"].value("ItemId", resource.ItemId);
      resource.Amount = entityData["ResourceNode"].value("Amount", resource.Amount);
      resource.MaxAmount = entityData["ResourceNode"].value("MaxAmount", resource.MaxAmount);
      resource.YieldPerInteract = entityData["ResourceNode"].value("YieldPerInteract", resource.YieldPerInteract);
      resource.RespawnDelay = entityData["ResourceNode"].value("RespawnDelay", resource.RespawnDelay);
      resource.RespawnTimer = entityData["ResourceNode"].value("RespawnTimer", resource.RespawnTimer);
      resource.Depleted = entityData["ResourceNode"].value("Depleted", resource.Depleted);
      world_.AddComponent(entity, std::move(resource));
    }

    if (entityData.contains("BuildPlacement")) {
      ecs::BuildPlacementComponent placement;
      placement.BlocksPath = entityData["BuildPlacement"].value("BlocksPath", placement.BlocksPath);
      placement.DisallowBlockingPath = entityData["BuildPlacement"].value("DisallowBlockingPath", placement.DisallowBlockingPath);
      placement.PlacementCooldown = entityData["BuildPlacement"].value("PlacementCooldown", placement.PlacementCooldown);
      placement.PlacementTimer = entityData["BuildPlacement"].value("PlacementTimer", placement.PlacementTimer);
      placement.RemainingPlacements = entityData["BuildPlacement"].value("RemainingPlacements", placement.RemainingPlacements);
      if (entityData["BuildPlacement"].contains("DefenseColor")) {
        auto& color = entityData["BuildPlacement"]["DefenseColor"];
        placement.DefenseColor = {(float)color[0], (float)color[1], (float)color[2], (float)color[3]};
      }
      world_.AddComponent(entity, placement);
    }

    if (entityData.contains("WaveSpawner")) {
      ecs::WaveSpawnerComponent spawner;
      if (entityData["WaveSpawner"].contains("SpawnCell")) {
        auto& cell = entityData["WaveSpawner"]["SpawnCell"];
        spawner.SpawnCell = {(int)cell[0], (int)cell[1]};
      }
      if (entityData["WaveSpawner"].contains("GoalCell")) {
        auto& cell = entityData["WaveSpawner"]["GoalCell"];
        spawner.GoalCell = {(int)cell[0], (int)cell[1]};
      }
      spawner.SpawnInterval = entityData["WaveSpawner"].value("SpawnInterval", spawner.SpawnInterval);
      spawner.SpawnTimer = entityData["WaveSpawner"].value("SpawnTimer", spawner.SpawnTimer);
      spawner.WaveInterval = entityData["WaveSpawner"].value("WaveInterval", spawner.WaveInterval);
      spawner.WaveTimer = entityData["WaveSpawner"].value("WaveTimer", spawner.WaveTimer);
      spawner.EnemySpeed = entityData["WaveSpawner"].value("EnemySpeed", spawner.EnemySpeed);
      spawner.EnemyHealth = entityData["WaveSpawner"].value("EnemyHealth", spawner.EnemyHealth);
      spawner.EnemiesPerWave = entityData["WaveSpawner"].value("EnemiesPerWave", spawner.EnemiesPerWave);
      spawner.RemainingInWave = entityData["WaveSpawner"].value("RemainingInWave", spawner.RemainingInWave);
      spawner.LoopWaves = entityData["WaveSpawner"].value("LoopWaves", spawner.LoopWaves);
      if (entityData["WaveSpawner"].contains("EnemyColor")) {
        auto& color = entityData["WaveSpawner"]["EnemyColor"];
        spawner.EnemyColor = {(float)color[0], (float)color[1], (float)color[2], (float)color[3]};
      }
      world_.AddComponent(entity, spawner);
    }

    if (entityData.contains("DefenseTower")) {
      ecs::DefenseTowerComponent tower;
      tower.Range = entityData["DefenseTower"].value("Range", tower.Range);
      tower.FireInterval = entityData["DefenseTower"].value("FireInterval", tower.FireInterval);
      tower.Cooldown = entityData["DefenseTower"].value("Cooldown", tower.Cooldown);
      tower.Damage = entityData["DefenseTower"].value("Damage", tower.Damage);
      world_.AddComponent(entity, tower);
    }

    if (entityData.contains("PathAgent")) {
      ecs::PathAgentComponent agent;
      if (entityData["PathAgent"].contains("GoalCell")) {
        auto& goal = entityData["PathAgent"]["GoalCell"];
        agent.GoalCell = {(int)goal[0], (int)goal[1]};
      }
      agent.MoveSpeed = entityData["PathAgent"].value("MoveSpeed", agent.MoveSpeed);
      agent.NextWaypointIndex = entityData["PathAgent"].value("NextWaypointIndex", agent.NextWaypointIndex);
      agent.DestroyAtGoal = entityData["PathAgent"].value("DestroyAtGoal", agent.DestroyAtGoal);
      agent.RepathRequired = entityData["PathAgent"].value("RepathRequired", agent.RepathRequired);
      if (entityData["PathAgent"].contains("Path")) {
        for (auto& cell : entityData["PathAgent"]["Path"]) {
          agent.Path.push_back({(int)cell[0], (int)cell[1]});
        }
      }
      world_.AddComponent(entity, std::move(agent));
    }

    // Deserialize Parent (by UUID)
    if (entityData.contains("Parent")) {
      UUID parentUUID = entityData["Parent"].get<uint64_t>();
      pendingParents.emplace_back(entity, parentUUID);
    }
  }

  for (const auto& [entity, parentUUID] : pendingParents) {
    ecs::Entity parent = world_.GetEntityByUUID(parentUUID);
    if (parent != ecs::INVALID_ENTITY) {
      world_.SetParent(entity, parent);
    }
  }

  GE_LOG_INFO("Scene deserialized from %s", filepath.c_str());
  return true;
}

bool SceneSerializer::SerializeWithHeader(const std::string& filepath, const savegame::SaveGameHeader& header) {
    json root;
    root["Header"] = {
        {"version", header.version},
        {"timestamp", header.timestamp},
        {"sceneName", header.sceneName},
        {"engineVersion", header.engineVersion}
    };
    root["Scene"] = header.sceneName;
    root["Entities"] = json::array();

    try {
        for (auto const& entity : world_.Query<ecs::TagComponent>()) {
            json entityJson;
            
            if (world_.HasComponent<ecs::IDComponent>(entity)) {
                entityJson["UUID"] = (uint64_t)world_.GetComponent<ecs::IDComponent>(entity).ID;
            }
            if (world_.HasComponent<ecs::TagComponent>(entity)) {
                entityJson["Tag"] = world_.GetComponent<ecs::TagComponent>(entity).tag;
            }
            if (world_.HasComponent<ecs::TransformComponent>(entity)) {
                auto& tc = world_.GetComponent<ecs::TransformComponent>(entity);
                entityJson["Transform"] = {
                    {"Translation", {tc.position.x, tc.position.y, tc.position.z}},
                    {"Rotation", {tc.rotation.w, tc.rotation.x, tc.rotation.y, tc.rotation.z}},
                    {"Scale", {tc.scale.x, tc.scale.y, tc.scale.z}}
                };
            }
            root["Entities"].push_back(entityJson);
        }

        std::string jsonStr = root.dump(4);
        uint32_t checksum = savegame::SaveGameChecksum::Calculate(jsonStr);

        std::ofstream fout(filepath, std::ios::binary);
        if (!fout.is_open()) {
            GE_LOG_ERROR("Could not open file for scene serialization: %s", filepath.c_str());
            return false;
        }

        savegame::SaveGameHeader outHeader = header;
        outHeader.payloadSize = static_cast<uint32_t>(jsonStr.size());
        outHeader.checksum = checksum;
        outHeader.sceneDataSize = outHeader.payloadSize;

        fout.write(reinterpret_cast<const char*>(&outHeader), sizeof(savegame::SaveGameHeader));
        fout.write(jsonStr.c_str(), jsonStr.size());

        GE_LOG_INFO("Scene serialized with header to %s (version: %u, checksum: %u)", 
                    filepath.c_str(), outHeader.version, outHeader.checksum);
        return true;
    } catch (const std::exception& e) {
        GE_LOG_ERROR("Scene serialization with header failed: %s", e.what());
        return false;
    }
}

bool SceneSerializer::DeserializeWithHeader(const std::string& filepath, savegame::SaveGameHeader& outHeader) {
    std::ifstream fin(filepath, std::ios::binary);
    if (!fin.is_open()) {
        GE_LOG_ERROR("Could not open save file: %s", filepath.c_str());
        return false;
    }

    fin.read(reinterpret_cast<char*>(&outHeader), sizeof(savegame::SaveGameHeader));

    if (!outHeader.IsCompatible()) {
        GE_LOG_ERROR("Incompatible save version: %u (current: %u)", 
                     outHeader.version, savegame::SAVEGAME_CURRENT_VERSION);
        return false;
    }

    std::string jsonData(outHeader.sceneDataSize, ' ');
    fin.read(&jsonData[0], outHeader.sceneDataSize);

    uint32_t computedChecksum = savegame::SaveGameChecksum::Calculate(jsonData);
    if (computedChecksum != outHeader.checksum) {
        GE_LOG_ERROR("Save file checksum mismatch! Expected: %u, Got: %u", 
                     outHeader.checksum, computedChecksum);
        return false;
    }

    json data = json::parse(jsonData);
    
    if (!data.contains("Entities")) {
        GE_LOG_ERROR("Save file has no entities");
        return false;
    }

    world_.Clear();

    for (auto& entityData : data["Entities"]) {
        ecs::Entity entity;
        if (entityData.contains("UUID")) {
            uint64_t uuid = entityData["UUID"];
            entity = world_.CreateEntityWithUUID(uuid);
        } else {
            entity = world_.CreateEntity();
        }

        if (entityData.contains("Tag")) {
            world_.AddComponent(entity, ecs::TagComponent{entityData["Tag"]});
        }
        if (entityData.contains("Transform")) {
            auto& tData = entityData["Transform"]["Translation"];
            auto& rData = entityData["Transform"]["Rotation"];
            auto& sData = entityData["Transform"]["Scale"];

            ecs::TransformComponent tc;
            tc.position = {(float)tData[0], (float)tData[1], (float)tData[2]};
            tc.rotation = {(float)rData[0], (float)rData[1], (float)rData[2], (float)rData[3]};
            tc.scale = {(float)sData[0], (float)sData[1], (float)sData[2]};
            world_.AddComponent(entity, tc);
        }
    }

    GE_LOG_INFO("Scene deserialized from %s (version: %u)", filepath.c_str(), outHeader.version);
    return true;
}

std::string SceneSerializer::SerializeToString() {
    json root;
    root["Scene"] = "Untitled";
    root["Version"] = savegame::SAVEGAME_CURRENT_VERSION;
    root["Entities"] = json::array();

    for (auto const& entity : world_.Query<ecs::TagComponent>()) {
        json entityJson;
        if (world_.HasComponent<ecs::IDComponent>(entity)) {
            entityJson["UUID"] = (uint64_t)world_.GetComponent<ecs::IDComponent>(entity).ID;
        }
        if (world_.HasComponent<ecs::TagComponent>(entity)) {
            entityJson["Tag"] = world_.GetComponent<ecs::TagComponent>(entity).tag;
        }
        if (world_.HasComponent<ecs::TransformComponent>(entity)) {
            auto& tc = world_.GetComponent<ecs::TransformComponent>(entity);
            entityJson["Transform"] = {
                {"Translation", {tc.position.x, tc.position.y, tc.position.z}},
                {"Rotation", {tc.rotation.w, tc.rotation.x, tc.rotation.y, tc.rotation.z}},
                {"Scale", {tc.scale.x, tc.scale.y, tc.scale.z}}
            };
        }
        root["Entities"].push_back(entityJson);
    }

    return root.dump(4);
}

bool SceneSerializer::DeserializeFromString(const std::string& data) {
    try {
        json root = json::parse(data);
        
        if (!root.contains("Entities")) {
            GE_LOG_ERROR("Invalid scene data: no Entities field");
            return false;
        }

        world_.Clear();

        for (auto& entityData : root["Entities"]) {
            ecs::Entity entity;
            if (entityData.contains("UUID")) {
                uint64_t uuid = entityData["UUID"];
                entity = world_.CreateEntityWithUUID(uuid);
            } else {
                entity = world_.CreateEntity();
            }

            if (entityData.contains("Tag")) {
                world_.AddComponent(entity, ecs::TagComponent{entityData["Tag"]});
            }
            if (entityData.contains("Transform")) {
                auto& tData = entityData["Transform"]["Translation"];
                auto& rData = entityData["Transform"]["Rotation"];
                auto& sData = entityData["Transform"]["Scale"];

                ecs::TransformComponent tc;
                tc.position = {(float)tData[0], (float)tData[1], (float)tData[2]};
                tc.rotation = {(float)rData[0], (float)rData[1], (float)rData[2], (float)rData[3]};
                tc.scale = {(float)sData[0], (float)sData[1], (float)sData[2]};
                world_.AddComponent(entity, tc);
            }
        }

        return true;
    } catch (const std::exception& e) {
        GE_LOG_ERROR("Scene deserialization from string failed: %s", e.what());
        return false;
    }
}

} // namespace scene
} // namespace ge
