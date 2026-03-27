#pragma once

#include "../../math/VecTypes.h"
#include <functional>

// Forward declarations for Jolt types to avoid including Jolt headers everywhere
namespace JPH {
    class CharacterVirtual;
}

namespace ge {
namespace ecs {

    /**
     * @brief Component for an advanced kinematic 3D Character Controller (using Jolt physics).
     */
    struct CharacterController3DComponent {
        // Physical dimensions
        float Height = 1.8f;      // Total character height
        float Radius = 0.4f;      // Character width/radius
        float Mass = 70.0f;       // Character physical mass

        // Controller Settings
        float MaxSlopeAngle = 45.0f;     // Maximum walkable slope angle in degrees
        float MaxStrength = 100.0f;      // Maximum force the character can apply to push other bodies
        float CharacterPadding = 0.02f;  // Padding added to the shape to prevent getting stuck

        // Movement Settings
        Math::Vec3f LinearVelocity = {0.0f, 0.0f, 0.0f}; // Intended movement velocity
        
        // State details
        bool IsGrounded = false;
        
        // Runtime Data (populated by Physics3DSystem)
        JPH::CharacterVirtual* RuntimeCharacter = nullptr;
    };

} // namespace ecs
} // namespace ge
