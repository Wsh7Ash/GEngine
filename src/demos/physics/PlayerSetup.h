#pragma once

// ================================================================
//  PlayerSetup.h
//  Player entity setup for physics showcase demo.
// ================================================================

#include "../../ecs/components/CharacterController3DComponent.h"
#include "../../ecs/components/Collider3DComponent.h"
#include "../../ecs/components/Rigidbody3DComponent.h"
#include "../../ecs/components/InputStateComponent.h"
#include "../../ecs/components/TransformComponent.h"
#include "../../ecs/World.h"
#include <cstdint>

namespace ge {
namespace demo {

struct PhysicsDemoPlayerConfig {
    float height = 1.8f;
    float radius = 0.4f;
    float mass = 70.0f;
    float moveSpeed = 5.0f;
    float runMultiplier = 2.0f;
    float crouchMultiplier = 0.5f;
    float jumpForce = 5.0f;
    float maxSlopeAngle = 45.0f;
};

inline ecs::Entity CreatePlayerEntity(ecs::World& world, const PhysicsDemoPlayerConfig& config = {}) {
    ecs::Entity player = world.CreateEntity();

    world.AddComponent(player, TransformComponent{
        Math::Vec3f(0.0f, 2.0f, 0.0f),
        Math::Quatf::Identity(),
        Math::Vec3f(1.0f, 1.0f, 1.0f)
    });

    world.AddComponent(player, CharacterController3DComponent{
        config.height,
        config.radius,
        config.mass,
        config.maxSlopeAngle,
        Math::Vec3f(0.0f, 0.0f, 0.0f),
        false
    });

    world.AddComponent(player, InputStateComponent{
        ge::demo::MovementState::Idle,
        ge::demo::InputAxis{0.0f, 0.0f},
        ge::demo::InputAxis{0.0f, 0.0f},
        Math::Vec2f(0.0f, 0.0f),
        false, false, false, false, false,
        false, false, false, false, false,
        config.moveSpeed,
        config.runMultiplier,
        config.crouchMultiplier,
        0.0f,
        Math::Vec3f(0.0f, 0.0f, 0.0f)
    });

    return player;
}

inline ecs::Entity CreateStaticBox(ecs::World& world, const Math::Vec3f& position, 
                                    const Math::Vec3f& size) {
    ecs::Entity box = world.CreateEntity();

    world.AddComponent(box, TransformComponent{
        position,
        Math::Quatf::Identity(),
        size
    });

    world.AddComponent(box, Collider3DComponent{
        ge::ecs::ColliderShape::Box,
        Math::Vec3f(0.0f, 0.0f, 0.0f),
        Math::Quatf::Identity(),
        size * 0.5f,
        0.5f,
        0.3f,
        false
    });

    world.AddComponent(box, Rigidbody3DComponent{
        ge::ecs::MotionType::Static,
        0.0f,
        0.5f,
        0.3f,
        0.0f,
        0.0f,
        1,
        1
    });

    return box;
}

inline ecs::Entity CreateDynamicBox(ecs::World& world, const Math::Vec3f& position,
                                     const Math::Vec3f& size, float mass = 1.0f) {
    ecs::Entity box = world.CreateEntity();

    world.AddComponent(box, TransformComponent{
        position,
        Math::Quatf::Identity(),
        size
    });

    world.AddComponent(box, Collider3DComponent{
        ge::ecs::ColliderShape::Box,
        Math::Vec3f(0.0f, 0.0f, 0.0f),
        Math::Quatf::Identity(),
        size * 0.5f,
        0.5f,
        0.3f,
        false
    });

    world.AddComponent(box, Rigidbody3DComponent{
        ge::ecs::MotionType::Dynamic,
        mass,
        0.5f,
        0.3f,
        0.0f,
        0.0f,
        1,
        1
    });

    return box;
}

inline ecs::Entity CreateRamp(ecs::World& world, const Math::Vec3f& position,
                               float width, float length, float angleDegrees) {
    ecs::Entity ramp = world.CreateEntity();

    float angleRad = angleDegrees * 3.14159f / 180.0f;
    float height = length * std::sin(angleRad);
    float thickness = 0.2f;

    world.AddComponent(ramp, TransformComponent{
        position,
        Math::Quatf::FromAxisAngle(Math::Vec3f(1.0f, 0.0f, 0.0f), -angleRad),
        Math::Vec3f(width, thickness, length)
    });

    world.AddComponent(ramp, Collider3DComponent{
        ge::ecs::ColliderShape::Box,
        Math::Vec3f(0.0f, 0.0f, 0.0f),
        Math::Quatf::Identity(),
        Math::Vec3f(width * 0.5f, thickness * 0.5f, length * 0.5f),
        0.5f,
        0.3f,
        false
    });

    world.AddComponent(ramp, Rigidbody3DComponent{
        ge::ecs::MotionType::Static,
        0.0f,
        0.5f,
        0.3f,
        0.0f,
        0.0f,
        1,
        1
    });

    return ramp;
}

} // namespace demo
} // namespace ge