#pragma once

#include "../../math/VecTypes.h"
#include "../../math/quaternion.h"

namespace ge {
namespace ecs {

    enum class Rigidbody3DMotionType {
        Static = 0,
        Kinematic,
        Dynamic
    };

    /**
     * @brief Component for 3D physics rigid bodies.
     */
    struct Rigidbody3DComponent {
        Rigidbody3DMotionType MotionType = Rigidbody3DMotionType::Dynamic;
        
        float Mass = 1.0f;
        float Friction = 0.5f;
        float Restitution = 0.0f;
        
        float LinearDamping = 0.05f;
        float AngularDamping = 0.05f;

        bool AllowSleeping = true;
        bool Sensor = false; // If true, it detects collisions but doesn't react physically

        // Runtime data (populated by Physics3DSystem)
        void* RuntimeBody = nullptr; 
    };

} // namespace ecs
} // namespace ge
