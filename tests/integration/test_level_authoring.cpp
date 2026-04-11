#include "../catch_amalgamated.hpp"

#include "../../src/core/editor/PrecisionEditTool.h"
#include "../../src/core/ecs/World.h"
#include "../../src/core/ecs/components/TagComponent.h"
#include "../../src/core/ecs/components/TilemapComponent.h"
#include "../../src/core/ecs/components/TransformComponent.h"
#include "../../src/core/input/InputManager.h"
#include "../../src/core/scene/SceneSerializer.h"

#include <filesystem>

namespace {

ge::ecs::Entity FindByTag(ge::ecs::World& world, const std::string& tag) {
    for (auto entity : world.Query<ge::ecs::TagComponent>()) {
        if (world.GetComponent<ge::ecs::TagComponent>(entity).tag == tag) {
            return entity;
        }
    }

    return ge::ecs::INVALID_ENTITY;
}

} // namespace

TEST_CASE("Level Authoring - Precision Edit Snap Values", "[authoring][transform]") {
    ge::editor::PrecisionEditTool tool;
    ge::editor::SnapSettings settings;
    settings.snapEnabled = true;
    settings.positionSnap = 0.5f;
    settings.rotationSnap = 15.0f;
    settings.scaleSnap = 0.25f;
    tool.SetSnapSettings(settings);

    REQUIRE(tool.SnapPosition(1.24f) == Catch::Approx(1.0f));
    REQUIRE(tool.SnapPosition(Math::Vec3f{1.24f, 2.26f, 0.0f}).y == Catch::Approx(2.5f));
    REQUIRE(tool.SnapRotation(22.0f) == Catch::Approx(15.0f));
    REQUIRE(tool.SnapScale(1.37f) == Catch::Approx(1.25f));
}

TEST_CASE("Level Authoring - Input Manager Builds Actions And Axis State", "[authoring][input]") {
    auto& input = ge::input::InputManager::Get();
    input.Shutdown();
    input.Initialize();
    input.EnablePlatformPolling(false);
    input.SetContextEnabled(ge::input::InputContext::Gameplay, false);

    REQUIRE_FALSE(input.IsContextEnabled(ge::input::InputContext::Gameplay));

    input.SetContextEnabled(ge::input::InputContext::Gameplay, true);
    input.OnKeyPressed(ge::input::KeyCode::E);
    input.Update();

    REQUIRE(input.IsActionPressed("Interact"));
    REQUIRE(input.IsActionJustPressed("Interact"));

    input.OnKeyPressed(ge::input::KeyCode::W);
    input.OnKeyPressed(ge::input::KeyCode::D);
    input.Update();

    const auto moveAxis = input.GetAxis2D("Move");
    REQUIRE(moveAxis.x == Catch::Approx(1.0f));
    REQUIRE(moveAxis.y == Catch::Approx(1.0f));

    input.OnKeyReleased(ge::input::KeyCode::E);
    input.Update();
    REQUIRE_FALSE(input.IsActionPressed("Interact"));
    REQUIRE(input.IsActionJustReleased("Interact"));

    input.Shutdown();
}

TEST_CASE("Level Authoring - Tilemap Scene State Round Trips", "[authoring][scene]") {
    ge::ecs::World world;
    const auto entity = world.CreateEntity();
    world.AddComponent(entity, ge::ecs::TagComponent{"TilemapRoot"});
    world.AddComponent(entity, ge::ecs::TransformComponent{});

    ge::ecs::TilemapComponent tilemap;
    tilemap.Width = 3;
    tilemap.Height = 2;
    tilemap.TileWidth = 16;
    tilemap.TileHeight = 16;
    tilemap.TilesPerRow = 4;
    tilemap.PixelsPerUnit = 16.0f;
    tilemap.ChunkWidth = 8;
    tilemap.ChunkHeight = 8;
    tilemap.AutoBuildNavigation = true;
    tilemap.TilePalette = {
        {0.10f, 0.20f, 0.30f, 1.0f},
        {0.80f, 0.70f, 0.20f, 1.0f}
    };

    ge::ecs::TilemapLayer ground;
    ground.Name = "Ground";
    ground.Visible = true;
    ground.CollisionLayer = false;
    ground.ZOffset = 0.0f;
    ground.Tiles = {0, 1, 0, 1, 0, 1};

    ge::ecs::TilemapLayer collision;
    collision.Name = "Collision";
    collision.Visible = true;
    collision.CollisionLayer = true;
    collision.ZOffset = 0.2f;
    collision.Tiles = {-1, -1, 1, -1, 1, -1};

    tilemap.Layers.push_back(ground);
    tilemap.Layers.push_back(collision);
    world.AddComponent(entity, tilemap);

    ge::scene::SceneSerializer serializer(world);
    const auto tempPath = std::filesystem::temp_directory_path() / "ge_level_authoring_roundtrip.json";
    REQUIRE(serializer.Serialize(tempPath.string()));

    ge::ecs::World reloadedWorld;
    ge::scene::SceneSerializer reloadedSerializer(reloadedWorld);
    REQUIRE(reloadedSerializer.Deserialize(tempPath.string()));
    std::error_code ec;
    std::filesystem::remove(tempPath, ec);

    const auto reloadedEntity = FindByTag(reloadedWorld, "TilemapRoot");
    REQUIRE(reloadedEntity != ge::ecs::INVALID_ENTITY);
    REQUIRE(reloadedWorld.HasComponent<ge::ecs::TilemapComponent>(reloadedEntity));

    const auto& reloadedTilemap = reloadedWorld.GetComponent<ge::ecs::TilemapComponent>(reloadedEntity);
    REQUIRE(reloadedTilemap.Width == 3);
    REQUIRE(reloadedTilemap.Height == 2);
    REQUIRE(reloadedTilemap.Layers.size() == 2);
    REQUIRE(reloadedTilemap.Layers[1].CollisionLayer);
    REQUIRE(reloadedTilemap.Layers[0].Tiles[1] == 1);
    REQUIRE(reloadedTilemap.TilePalette.size() == 2);
    REQUIRE(reloadedTilemap.TilePalette[1].x == Catch::Approx(0.80f));
    REQUIRE(reloadedTilemap.Navigation.Width == 3);
    REQUIRE(reloadedTilemap.Navigation.Height == 2);
}
