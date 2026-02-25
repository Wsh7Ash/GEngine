#include <cstdio>
#include <vector>
#include "../src/core/ge_core.h"

using namespace ge;
using namespace ge::ecs;

// simple movement system
struct MovementSystem : public System
{
    void Update(World& world, float dt)
    {
        for (auto const& entity : entities)
        {
            auto& transform = world.GetComponent<TransformComponent>(entity);
            auto& velocity = world.GetComponent<VelocityComponent>(entity);
            transform.position += velocity.velocity * dt;
        }
    }
};

int main()
{
    printf("ECS Test Starting...\n");

    World world;

    // Register components
    // (Happens automatically now on first use)

    // Register system
    auto movementSystem = world.RegisterSystem<MovementSystem>();
    {
        Signature signature;
        signature.set(GetComponentTypeID<TransformComponent>());
        signature.set(GetComponentTypeID<VelocityComponent>());
        world.SetSystemSignature<MovementSystem>(signature);
    }

    // Create entities
    std::vector<Entity> entities;
    for (int i = 0; i < 10; ++i)
    {
        Entity e = world.CreateEntity();
        world.AddComponent(e, TransformComponent{ Math::Vec3f(0, 0, 0) });
        
        if (i % 2 == 0) {
            world.AddComponent(e, VelocityComponent{ Math::Vec3f(1, 0, 0) });
        }
        
        entities.push_back(e);
    }

    printf("Entities created. System has %zu entities.\n", movementSystem->entities.size());

    // Run system
    movementSystem->Update(world, 1.0f);

    // Verify results
    for (int i = 0; i < 10; ++i)
    {
        auto& t = world.GetComponent<TransformComponent>(entities[i]);
        if (i % 2 == 0)
        {
            if (t.position.x != 1.0f) {
                printf("FAIL: Entity %d moved incorrectly (x=%.2f)\n", i, t.position.x);
                return 1;
            }
        }
        else
        {
            if (t.position.x != 0.0f) {
                printf("FAIL: Entity %d moved without velocity (x=%.2f)\n", i, t.position.x);
                return 1;
            }
        }
    }

    printf("MovementSystem test PASSED.\n");

    // Test Queries
    printf("Testing Queries...\n");
    int count = 0;
    for (auto e : world.Query<TransformComponent, VelocityComponent>())
    {
        (void)e;
        count++;
    }

    if (count != 5) {
        printf("FAIL: Query returned %d entities, expected 5\n", count);
        return 1;
    }
    printf("Query test PASSED.\n");

    // Test Destruction
    Entity toDestroy = entities[0];
    world.DestroyEntity(toDestroy);

    if (movementSystem->entities.count(toDestroy)) {
        printf("FAIL: Destroyed entity still in system\n");
        return 1;
    }
    printf("Destruction test PASSED.\n");

    printf("ALL ECS TESTS PASSED!\n");
    return 0;
}
