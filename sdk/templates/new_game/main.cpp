#include <GEngine/GEngine.h>

int main() {
    ge::platform::Initialize();

    ge::ecs::World world;

    auto player = world.CreateEntity();
    player.AddComponent<ge::ecs::TransformComponent>();
    auto& velocity = player.AddComponent<ge::ecs::VelocityComponent>();
    velocity.Linear = Math::Vec3f{1.0f, 0.0f, 0.0f};

    GE_LOG_INFO("Created player entity with velocity");

    for (int i = 0; i < 60; ++i) {
        world.ForEach<ge::ecs::TransformComponent, ge::ecs::VelocityComponent>(
            [](ge::ecs::TransformComponent& transform, const ge::ecs::VelocityComponent& vel) {
                transform.Position += vel.Linear * 0.016f;
            });
    }

    ge::platform::Shutdown();
    return 0;
}
