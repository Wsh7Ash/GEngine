#pragma once

#include "../../math/VecTypes.h"
#include <variant>

namespace ge {
namespace ecs {

    enum class Collider3DShapeType {
        Box,
        Sphere,
        Capsule,
        TriangleMesh,
        ConvexHull
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

    /**
     * @brief Component for 3D physical colliders.
     */
    struct Collider3DComponent {
        Collider3DShapeType ShapeType = Collider3DShapeType::Box;
        
        // Use a variant to store shape data? Or just fixed fields? 
        // For simplicity in editor UI, fixed fields might be better.
        Math::Vec3f BoxHalfExtents = { 0.5f, 0.5f, 0.5f };
        float SphereRadius = 0.5f;
        float CapsuleRadius = 0.25f;
        float CapsuleHalfHeight = 0.5f;

        Math::Vec3f Offset = { 0.0f, 0.0f, 0.0f };
        bool IsTrigger = false;

        // Material properties
        float Friction = 0.5f;
        float Restitution = 0.0f;
    };

} // namespace ecs
} // namespace ge
