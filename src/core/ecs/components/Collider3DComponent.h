#pragma once

#include "../../math/VecTypes.h"
#include <variant>
#include <vector>

namespace ge {
namespace ecs {

    enum class Collider3DShapeType {
        Box,
        Sphere,
        Capsule,
        TriangleMesh,
        ConvexHull,
        HeightField
    };

    struct BoxShape {
        Math::Vec3f HalfExtents = { 0.5f, 0.5f, 0.5f };
    };

    struct SphereShape {
        float Radius = 0.5f;
    };

    struct CapsuleShape {
        float Radius = 0.25f;
        float HalfHeight = 0.5f;
    };

    struct HeightFieldShape {
        uint32_t Width = 0;
        uint32_t Depth = 0;
        float ScaleX = 1.0f;
        float ScaleZ = 1.0f;
        float HeightScale = 1.0f;
        float OffsetY = 0.0f;
        std::vector<float> Heights;
    };

    /**
     * @brief Component for 3D physical colliders.
     */
    struct Collider3DComponent {
        Collider3DShapeType ShapeType = Collider3DShapeType::Box;
        
        Math::Vec3f BoxHalfExtents = { 0.5f, 0.5f, 0.5f };
        float SphereRadius = 0.5f;
        float CapsuleRadius = 0.25f;
        float CapsuleHalfHeight = 0.5f;
        
        HeightFieldShape HeightField;

        Math::Vec3f Offset = { 0.0f, 0.0f, 0.0f };
        bool IsTrigger = false;

        float Friction = 0.5f;
        float Restitution = 0.0f;

        float lagCompensationPadding = 0.0f;
    };

} // namespace ecs
} // namespace ge
