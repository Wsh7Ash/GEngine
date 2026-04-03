#include "catch_amalgamated.hpp"
#include "../src/core/ecs/World.h"
#include "../src/core/ecs/components/TransformComponent.h"
#include "../src/core/ecs/components/VelocityComponent.h"
#include "../src/core/ecs/systems/Physics2DSystem.h"

TEST_CASE("World Create Entity", "[ecs]")
{
    ge::ecs::World world;
    auto entity = world.CreateEntity();
    REQUIRE(entity.IsValid());
}

TEST_CASE("World Create Multiple Entities", "[ecs]")
{
    ge::ecs::World world;
    auto e1 = world.CreateEntity();
    auto e2 = world.CreateEntity();
    auto e3 = world.CreateEntity();
    
    REQUIRE(e1.IsValid());
    REQUIRE(e2.IsValid());
    REQUIRE(e3.IsValid());
    REQUIRE(e1 != e2);
}

TEST_CASE("World Destroy Entity", "[ecs]")
{
    ge::ecs::World world;
    auto entity = world.CreateEntity();
    REQUIRE(entity.IsValid());
    
    world.DestroyEntity(entity);
    REQUIRE_FALSE(entity.IsValid());
}

TEST_CASE("World Add Component", "[ecs]")
{
    ge::ecs::World world;
    auto entity = world.CreateEntity();
    
    auto& transform = entity.AddComponent<ge::ecs::TransformComponent>();
    transform.Position = Math::Vec3f{1.0f, 2.0f, 3.0f};
    
    REQUIRE(entity.HasComponent<ge::ecs::TransformComponent>());
    REQUIRE(entity.GetComponent<ge::ecs::TransformComponent>().Position.x == 1.0f);
}

TEST_CASE("World Remove Component", "[ecs]")
{
    ge::ecs::World world;
    auto entity = world.CreateEntity();
    entity.AddComponent<ge::ecs::TransformComponent>();
    
    REQUIRE(entity.HasComponent<ge::ecs::TransformComponent>());
    
    entity.RemoveComponent<ge::ecs::TransformComponent>();
    REQUIRE_FALSE(entity.HasComponent<ge::ecs::TransformComponent>());
}

TEST_CASE("World Query Single Component", "[ecs]")
{
    ge::ecs::World world;
    
    auto e1 = world.CreateEntity();
    e1.AddComponent<ge::ecs::TransformComponent>();
    
    auto e2 = world.CreateEntity();
    e2.AddComponent<ge::ecs::TransformComponent>();
    
    auto e3 = world.CreateEntity();
    
    int count = 0;
    world.ForEach<ge::ecs::TransformComponent>([&count](ge::ecs::TransformComponent&) {
        count++;
    });
    REQUIRE(count == 2);
}

TEST_CASE("World Query Multiple Components", "[ecs]")
{
    ge::ecs::World world;
    
    for (int i = 0; i < 10; ++i) {
        auto entity = world.CreateEntity();
        entity.AddComponent<ge::ecs::TransformComponent>();
        if (i < 5) {
            entity.AddComponent<ge::ecs::VelocityComponent>();
        }
    }
    
    int count = 0;
    world.ForEach<ge::ecs::TransformComponent, ge::ecs::VelocityComponent>(
        [&count](ge::ecs::TransformComponent&, ge::ecs::VelocityComponent&) {
            count++;
        });
    REQUIRE(count == 5);
}

TEST_CASE("Physics2DSystem Integration", "[ecs][physics]")
{
    ge::ecs::World world;
    
    ge::ecs::Physics2DSystem physicsSystem;
    world.AddSystem<ge::ecs::Physics2DSystem>(physicsSystem);
    
    for (int i = 0; i < 10; ++i) {
        auto entity = world.CreateEntity();
        auto& transform = entity.AddComponent<ge::ecs::TransformComponent>();
        transform.Position = Math::Vec3f{static_cast<float>(i), 0.0f, 0.0f};
        
        if (i < 5) {
            auto& velocity = entity.AddComponent<ge::ecs::VelocityComponent>();
            velocity.Linear = Math::Vec3f{1.0f, 0.0f, 0.0f};
        }
    }
    
    physicsSystem.Update(world, 0.016f);
    
    int movedCount = 0;
    world.ForEach<ge::ecs::TransformComponent, ge::ecs::VelocityComponent>(
        [&movedCount](ge::ecs::TransformComponent& t, ge::ecs::VelocityComponent&) {
            if (t.Position.x != static_cast<float>(movedCount)) {
                movedCount++;
            }
        });
    REQUIRE(movedCount > 0);
}

TEST_CASE("Entity Destroy Removes From Query", "[ecs]")
{
    ge::ecs::World world;
    
    auto e1 = world.CreateEntity();
    e1.AddComponent<ge::ecs::TransformComponent>();
    e1.AddComponent<ge::ecs::VelocityComponent>();
    
    auto e2 = world.CreateEntity();
    e2.AddComponent<ge::ecs::TransformComponent>();
    e2.AddComponent<ge::ecs::VelocityComponent>();
    
    int count = 0;
    world.ForEach<ge::ecs::TransformComponent, ge::ecs::VelocityComponent>(
        [&count](ge::ecs::TransformComponent&, ge::ecs::VelocityComponent&) {
            count++;
        });
    REQUIRE(count == 2);
    
    world.DestroyEntity(e1);
    
    count = 0;
    world.ForEach<ge::ecs::TransformComponent, ge::ecs::VelocityComponent>(
        [&count](ge::ecs::TransformComponent&, ge::ecs::VelocityComponent&) {
            count++;
        });
    REQUIRE(count == 1);
}
