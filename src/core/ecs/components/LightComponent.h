#pragma once

#include "../../math/VecTypes.h"

namespace ge {
namespace ecs {

    /**
     * @brief Types of lights supported by the PBR renderer.
     */
    enum class LightType
    {
        Directional = 0,
        Point       = 1,
        Spot        = 2
    };

    /**
     * @brief Component that turns an entity into a light source.
     */
    struct LightComponent
    {
        LightType Type      = LightType::Directional;
        Math::Vec3f Color   = { 1.0f, 1.0f, 1.0f };
        float Intensity     = 1.0f;
        
        // Point/Spot light specific
        float Range         = 10.0f;
        
        // Spot light specific
        Math::Vec3f SpotDirection = { 0.0f, -1.0f, 0.0f };
        float SpotOuterCone = 45.0f;  // Degrees
        float SpotInnerCone = 30.0f;  // Degrees
        
        bool CastShadows    = true;
    };

} // namespace ecs
} // namespace ge
