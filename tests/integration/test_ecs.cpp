#include "catch_amalgamated.hpp"

#include "../../src/core/ecs/World.h"
#include "../../src/core/ecs/components/TagComponent.h"

TEST_CASE("ECS Query Iteration Reaches End Without Touching Packed Storage Out Of Bounds", "[ecs][query]") {
    ge::ecs::World world;

    for (int i = 0; i < 5; ++i) {
        const auto entity = world.CreateEntity();
        world.AddComponent(entity, ge::ecs::TagComponent{"Entity" + std::to_string(i)});
    }

    int visited = 0;
    for (auto entity : world.Query<ge::ecs::TagComponent>()) {
        REQUIRE(entity != ge::ecs::INVALID_ENTITY);
        ++visited;
    }

    REQUIRE(visited == 5);
}

TEST_CASE("ECS Query Survives Removing Queried Components Mid Iteration", "[ecs][query][mutation]") {
    ge::ecs::World world;

    for (int i = 0; i < 5; ++i) {
        const auto entity = world.CreateEntity();
        world.AddComponent(entity, ge::ecs::TagComponent{"Entity" + std::to_string(i)});
    }

    int visited = 0;
    bool deletedOne = false;

    for (auto entity : world.Query<ge::ecs::TagComponent>()) {
        REQUIRE(entity != ge::ecs::INVALID_ENTITY);
        ++visited;

        if (!deletedOne) {
            world.DestroyEntity(entity);
            deletedOne = true;
        }
    }

    int remaining = 0;
    for (auto entity : world.Query<ge::ecs::TagComponent>()) {
        REQUIRE(entity != ge::ecs::INVALID_ENTITY);
        ++remaining;
    }

    REQUIRE(deletedOne);
    REQUIRE(visited == 4);
    REQUIRE(remaining == 4);
}
