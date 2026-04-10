#include "../catch_amalgamated.hpp"

#include "../../src/core/ecs/World.h"
#include "../../src/core/ecs/components/BuildPlacementComponent.h"
#include "../../src/core/ecs/components/InventoryComponent.h"
#include "../../src/core/ecs/components/ResourceNodeComponent.h"
#include "../../src/core/ecs/components/TagComponent.h"
#include "../../src/core/ecs/components/TilemapComponent.h"
#include "../../src/core/ecs/components/TopDownControllerComponent.h"
#include "../../src/core/ecs/components/TransformComponent.h"
#include "../../src/core/gameplay/GridPathfinder.h"
#include "../../src/core/scene/SceneSerializer.h"

TEST_CASE("Pixel Foundation - Grid Path Around Obstacle", "[pixel][path]") {
    ge::gameplay::GridPathData grid;
    grid.Resize(5, 5, false);
    grid.SetBlocked({2, 1}, true);
    grid.SetBlocked({2, 2}, true);
    grid.SetBlocked({2, 3}, true);

    std::vector<ge::gameplay::GridCoord> path;
    const bool found = ge::gameplay::GridPathfinder::FindPath(grid, {0, 2}, {4, 2}, path);

    REQUIRE(found);
    REQUIRE(path.front() == ge::gameplay::GridCoord{0, 2});
    REQUIRE(path.back() == ge::gameplay::GridCoord{4, 2});
    REQUIRE(path.size() > 5);
}

TEST_CASE("Pixel Foundation - Grid Path Blocked", "[pixel][path]") {
    ge::gameplay::GridPathData grid;
    grid.Resize(3, 3, false);
    grid.SetBlocked({1, 0}, true);
    grid.SetBlocked({1, 1}, true);
    grid.SetBlocked({1, 2}, true);

    std::vector<ge::gameplay::GridCoord> path;
    const bool found = ge::gameplay::GridPathfinder::FindPath(grid, {0, 1}, {2, 1}, path);

    REQUIRE_FALSE(found);
    REQUIRE(path.empty());
}

TEST_CASE("Pixel Foundation - Scene Round Trip Preserves Tilemap And State", "[pixel][serialization]") {
    ge::ecs::World world;

    auto mapEntity = world.CreateEntity();
    world.AddComponent(mapEntity, ge::ecs::TagComponent{"Map"});
    world.AddComponent(mapEntity, ge::ecs::TransformComponent{});

    ge::ecs::TilemapComponent tilemap;
    tilemap.Width = 4;
    tilemap.Height = 3;
    tilemap.TileWidth = 16;
    tilemap.TileHeight = 16;
    tilemap.Navigation.Resize(4, 3, false);
    tilemap.Navigation.CellSize = 1.0f;
    tilemap.Navigation.Origin = {-2.0f, -1.5f};

    ge::ecs::TilemapLayer layer;
    layer.Name = "Ground";
    layer.Tiles = {0, 1, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1};
    tilemap.Layers.push_back(layer);
    tilemap.TilePalette = {{0.2f, 0.3f, 0.2f, 1.0f}, {0.4f, 0.3f, 0.2f, 1.0f}};
    world.AddComponent(mapEntity, tilemap);

    auto player = world.CreateEntity();
    world.AddComponent(player, ge::ecs::TagComponent{"Player"});
    world.AddComponent(player, ge::ecs::TransformComponent{});
    world.AddComponent(player, ge::ecs::TopDownControllerComponent{4.0f});

    ge::ecs::InventoryComponent inventory;
    inventory.Items.push_back({"resource.wood", 7});
    world.AddComponent(player, inventory);
    world.AddComponent(player, ge::ecs::BuildPlacementComponent{});

    auto node = world.CreateEntity();
    world.AddComponent(node, ge::ecs::TagComponent{"Node"});
    world.AddComponent(node, ge::ecs::TransformComponent{});
    world.AddComponent(node, ge::ecs::ResourceNodeComponent{"resource.wood", 3, 5, 1, 0.0f, 0.0f, false});

    ge::scene::SceneSerializer serializer(world);
    const std::string path = "pixel_foundation_roundtrip.json";
    REQUIRE(serializer.Serialize(path));

    world.Clear();
    REQUIRE(serializer.Deserialize(path));

    int tilemapCount = 0;
    int playerCount = 0;
    int nodeCount = 0;

    for (auto entity : world.Query<ge::ecs::TagComponent>()) {
        const auto& tag = world.GetComponent<ge::ecs::TagComponent>(entity).tag;
        if (tag == "Map") {
            tilemapCount++;
            REQUIRE(world.HasComponent<ge::ecs::TilemapComponent>(entity));
            const auto& loadedTilemap = world.GetComponent<ge::ecs::TilemapComponent>(entity);
            REQUIRE(loadedTilemap.Width == 4);
            REQUIRE(loadedTilemap.Height == 3);
            REQUIRE(loadedTilemap.Layers.size() == 1);
            REQUIRE(loadedTilemap.Layers[0].Tiles[1] == 1);
        } else if (tag == "Player") {
            playerCount++;
            REQUIRE(world.HasComponent<ge::ecs::InventoryComponent>(entity));
            const auto& loadedInventory = world.GetComponent<ge::ecs::InventoryComponent>(entity);
            REQUIRE(loadedInventory.Items.size() == 1);
            REQUIRE(loadedInventory.Items[0].ItemId == "resource.wood");
            REQUIRE(loadedInventory.Items[0].Quantity == 7);
        } else if (tag == "Node") {
            nodeCount++;
            REQUIRE(world.HasComponent<ge::ecs::ResourceNodeComponent>(entity));
            const auto& loadedNode = world.GetComponent<ge::ecs::ResourceNodeComponent>(entity);
            REQUIRE(loadedNode.Amount == 3);
            REQUIRE(loadedNode.MaxAmount == 5);
        }
    }

    REQUIRE(tilemapCount == 1);
    REQUIRE(playerCount == 1);
    REQUIRE(nodeCount == 1);
}
