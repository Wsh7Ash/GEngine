#pragma once

#include "../../math/VecTypes.h"

namespace ge {
namespace ecs {

    struct BoxCollider2DComponent
    {
        Math::Vec2f Offset = {0.0f, 0.0f};
        Math::Vec2f Size = {0.5f, 0.5f};

        // Physics material
        float Density = 1.0f;
        float Friction = 0.5f;
        float Restitution = 0.0f;
        float RestitutionThreshold = 0.5f;

        // Future: pointer to runtime fixture (e.g. b2Fixture*)
        void* RuntimeFixture = nullptr;

        BoxCollider2DComponent() = default;
        BoxCollider2DComponent(const BoxCollider2DComponent&) = default;
    };

} // namespace ecs
} // namespace ge
