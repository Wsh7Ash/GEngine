#include "../catch_amalgamated.hpp"

#include "../../src/core/ge_core.h"
#include "../../src/core/ecs/components/JointComponent.h"

TEST_CASE("Component Registry - By Name Supports Common Gameplay Components", "[ecs][components]") {
    ge::ecs::World world;
    const auto entity = world.CreateEntity();

    world.AddComponentByName(entity, "TransformComponent");
    world.AddComponentByName(entity, "Tag");
    world.AddComponentByName(entity, "InventoryComponent");
    world.AddComponentByName(entity, "GridMap");

    REQUIRE(world.HasComponent<ge::ecs::TransformComponent>(entity));
    REQUIRE(world.HasComponentByName(entity, "transform"));
    REQUIRE(world.HasComponentByName(entity, "ge::ecs::TagComponent"));
    REQUIRE(world.HasComponentByName(entity, "Inventory"));
    REQUIRE(world.HasComponentByName(entity, "GridMapComponent"));

    auto* transform = static_cast<ge::ecs::TransformComponent*>(
        world.GetComponentPointerByName(entity, "TransformComponent"));
    REQUIRE(transform != nullptr);
    transform->position.x = 5.0f;

    REQUIRE(world.GetComponent<ge::ecs::TransformComponent>(entity).position.x == Catch::Approx(5.0f));

    world.RemoveComponentByName(entity, "Inventory");
    REQUIRE_FALSE(world.HasComponentByName(entity, "InventoryComponent"));
}

TEST_CASE("Component Registry - Type Names Are Stable And Readable", "[ecs][components]") {
    const auto transformId = ge::ecs::GetComponentTypeID<ge::ecs::TransformComponent>();
    const auto waveSpawnerId = ge::ecs::GetComponentTypeID<ge::ecs::WaveSpawnerComponent>();

    REQUIRE(std::string(ge::ecs::World::GetComponentTypeNameStatic(transformId)) == "ge::ecs::TransformComponent");
    REQUIRE(std::string(ge::ecs::World::GetComponentTypeNameStatic(waveSpawnerId)) == "ge::ecs::WaveSpawnerComponent");
}

TEST_CASE("Component Headers - Standalone Utility Components Compile Cleanly", "[ecs][components][headers]") {
    ge::ecs::AudioSourceComponent audioSource;
    ge::ecs::PrefabOverrideComponent prefabOverride;
    ge::ecs::JointComponent joint;

    prefabOverride.AddOverride({"TransformComponent", "position", "[0,0,0]", "[1,2,0]"});
    REQUIRE(prefabOverride.HasOverrides());
    prefabOverride.RemoveOverride("TransformComponent", "position");
    REQUIRE_FALSE(prefabOverride.HasOverrides());

    audioSource.DistanceModel = ge::ecs::AudioDistanceModel::ExponentialDistance;
    REQUIRE(audioSource.CalculateAttenuation(2.0f) > 0.0f);

    joint.IsBroken = false;
    REQUIRE_FALSE(joint.IsBroken);
}
