#include "../catch_amalgamated.hpp"

#include "../../src/core/editor/EditorPaths.h"
#include "../../src/core/ecs/World.h"
#include "../../src/core/ecs/components/BoxCollider2DComponent.h"
#include "../../src/core/ecs/components/GridPathData.h"
#include "../../src/core/ecs/components/InventoryComponent.h"
#include "../../src/core/ecs/components/RelationshipComponent.h"
#include "../../src/core/ecs/components/Rigidbody2DComponent.h"
#include "../../src/core/ecs/components/ResourceNodeComponent.h"
#include "../../src/core/ecs/components/TagComponent.h"
#include "../../src/core/ecs/components/TilemapComponent.h"
#include "../../src/core/ecs/components/TopDownControllerComponent.h"
#include "../../src/core/ecs/components/TransformComponent.h"
#include "../../src/core/scene/SceneSerializer.h"

#include <filesystem>
#include <string>

namespace {

std::filesystem::path FixturePath(const std::string& name) {
    return ge::editor::EditorPaths::ResolveProjectRoot() / "tests" / "fixtures" / name;
}

ge::ecs::Entity FindByTag(ge::ecs::World& world, const std::string& tag) {
    for (auto entity : world.Query<ge::ecs::TagComponent>()) {
        if (world.GetComponent<ge::ecs::TagComponent>(entity).tag == tag) {
            return entity;
        }
    }

    return ge::ecs::INVALID_ENTITY;
}

} // namespace

TEST_CASE("Editor Regression - Project and Asset Roots Resolve", "[editor][paths]") {
    const auto projectRoot = ge::editor::EditorPaths::ResolveProjectRoot();
    const auto assetRoot = ge::editor::EditorPaths::ResolveAssetRoot();

    REQUIRE(std::filesystem::exists(projectRoot / "CMakeLists.txt"));
    REQUIRE(std::filesystem::exists(projectRoot / "src"));
    REQUIRE(std::filesystem::exists(assetRoot));
    REQUIRE(ge::editor::EditorPaths::LooksLikeSceneAsset(FixturePath("editor_flat_scene.json")));
}

TEST_CASE("Editor Regression - SetParent Rejects Cycles And Duplicates", "[editor][hierarchy]") {
    ge::ecs::World world;

    auto root = world.CreateEntity();
    world.AddComponent(root, ge::ecs::TagComponent{"Root"});

    auto child = world.CreateEntity();
    world.AddComponent(child, ge::ecs::TagComponent{"Child"});

    auto grandChild = world.CreateEntity();
    world.AddComponent(grandChild, ge::ecs::TagComponent{"GrandChild"});

    world.SetParent(child, root);
    world.SetParent(child, root);
    world.SetParent(grandChild, child);
    world.SetParent(root, grandChild);

    REQUIRE(world.HasComponent<ge::ecs::RelationshipComponent>(root));
    REQUIRE(world.HasComponent<ge::ecs::RelationshipComponent>(child));

    const auto& rootRelationship = world.GetComponent<ge::ecs::RelationshipComponent>(root);
    const auto& childRelationship = world.GetComponent<ge::ecs::RelationshipComponent>(child);

    REQUIRE(rootRelationship.Children.size() == 1);
    REQUIRE(rootRelationship.Children.front() == child);
    REQUIRE(childRelationship.Parent == root);
    REQUIRE(world.IsDescendantOf(grandChild, root));
    REQUIRE_FALSE(world.IsDescendantOf(root, grandChild));
}

TEST_CASE("Editor Regression - Flat Fixture Replaces Prior World State", "[editor][scene]") {
    ge::ecs::World world;
    auto stale = world.CreateEntity();
    world.AddComponent(stale, ge::ecs::TagComponent{"Stale"});

    ge::scene::SceneSerializer serializer(world);
    REQUIRE(serializer.Deserialize(FixturePath("editor_flat_scene.json").string()));

    REQUIRE(FindByTag(world, "Stale") == ge::ecs::INVALID_ENTITY);

    const auto map = FindByTag(world, "Map");
    const auto grid = FindByTag(world, "NavigationGrid");
    const auto player = FindByTag(world, "Player");
    const auto node = FindByTag(world, "TreeNode");

    REQUIRE(map != ge::ecs::INVALID_ENTITY);
    REQUIRE(grid != ge::ecs::INVALID_ENTITY);
    REQUIRE(player != ge::ecs::INVALID_ENTITY);
    REQUIRE(node != ge::ecs::INVALID_ENTITY);

    REQUIRE(world.HasComponent<ge::ecs::TilemapComponent>(map));
    REQUIRE(world.HasComponent<ge::ecs::GridMapComponent>(grid));
    REQUIRE(world.HasComponent<ge::ecs::TopDownControllerComponent>(player));
    REQUIRE(world.HasComponent<ge::ecs::InventoryComponent>(player));
    REQUIRE(world.HasComponent<ge::ecs::ResourceNodeComponent>(node));

    const auto& tilemap = world.GetComponent<ge::ecs::TilemapComponent>(map);
    REQUIRE(tilemap.Width == 4);
    REQUIRE(tilemap.Height == 4);
    REQUIRE(tilemap.Layers.size() == 1);
    REQUIRE(tilemap.Layers.front().Tiles.size() == 16);

    const auto& inventory = world.GetComponent<ge::ecs::InventoryComponent>(player);
    REQUIRE(inventory.Items.size() == 1);
    REQUIRE(inventory.Items.front().ItemId == "resource.wood");
    REQUIRE(inventory.Items.front().Quantity == 3);

    REQUIRE(world.ResolveEntityByIndex(player.GetIndex()) == player);
}

TEST_CASE("Editor Regression - Hierarchy Fixture Restores Deferred Parent Links", "[editor][scene][hierarchy]") {
    ge::ecs::World world;
    ge::scene::SceneSerializer serializer(world);

    REQUIRE(serializer.Deserialize(FixturePath("editor_hierarchy_scene.json").string()));

    const auto root = FindByTag(world, "Root");
    const auto child = FindByTag(world, "Child");
    const auto grandChild = FindByTag(world, "GrandChild");
    const auto detached = FindByTag(world, "Detached");

    REQUIRE(root != ge::ecs::INVALID_ENTITY);
    REQUIRE(child != ge::ecs::INVALID_ENTITY);
    REQUIRE(grandChild != ge::ecs::INVALID_ENTITY);
    REQUIRE(detached != ge::ecs::INVALID_ENTITY);

    REQUIRE(world.HasComponent<ge::ecs::RelationshipComponent>(root));
    REQUIRE(world.HasComponent<ge::ecs::RelationshipComponent>(child));

    const auto& rootRelationship = world.GetComponent<ge::ecs::RelationshipComponent>(root);
    const auto& childRelationship = world.GetComponent<ge::ecs::RelationshipComponent>(child);
    const auto& grandChildRelationship = world.GetComponent<ge::ecs::RelationshipComponent>(grandChild);

    REQUIRE(rootRelationship.Parent == ge::ecs::INVALID_ENTITY);
    REQUIRE(childRelationship.Parent == root);
    REQUIRE(grandChildRelationship.Parent == child);
    REQUIRE(rootRelationship.Children.size() == 1);
    REQUIRE(rootRelationship.Children.front() == child);
    REQUIRE(childRelationship.Children.size() == 1);
    REQUIRE(childRelationship.Children.front() == grandChild);
    REQUIRE(world.IsDescendantOf(grandChild, root));
}
