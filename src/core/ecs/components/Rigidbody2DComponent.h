#pragma once

namespace ge {
namespace ecs {

    enum class RigidBody2DType { Static = 0, Dynamic, Kinematic };

    struct Rigidbody2DComponent
    {
        RigidBody2DType Type = RigidBody2DType::Static;
        bool FixedRotation = false;

        // Future: pointer to runtime body (e.g. b2Body*)
        void* RuntimeBody = nullptr;

        Rigidbody2DComponent() = default;
        Rigidbody2DComponent(const Rigidbody2DComponent&) = default;
    };

} // namespace ecs
} // namespace ge
