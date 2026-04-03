#pragma once

// ================================================================
//  LightComponent.h
//  Component for light sources.
// ================================================================

#include "../../math/VecTypes.h"
#include "../../net/ReplicationAttributes.h"

namespace ge {
namespace ecs {

    enum class LightType
    {
        Directional = 0,
        Point       = 1,
        Spot        = 2
    };

    struct LightComponent
    {
        LightType Type      = LightType::Directional;
        Math::Vec3f Color   = { 1.0f, 1.0f, 1.0f };
        float Intensity     = 1.0f;
        
        // Point/Spot light specific
        float Range         = 10.0f;
        
        // Spot light specific
        Math::Vec3f SpotDirection = { 0.0f, -1.0f, 0.0f };
        float SpotOuterCone = 45.0f;
        float SpotInnerCone = 30.0f;
        
        bool CastShadows    = true;
        
        // CSM Shadow settings
        int ShadowCascadeCount = 4;
        float ShadowCascadeDistances[4] = { 1.0f, 25.0f, 100.0f, 500.0f };
        int ShadowMapSize = 2048;
        float ShadowBias = 0.001f;
        float ShadowDarkness = 0.3f;
        
        // Volumetric Fog settings
        bool VolumetricFog = true;
        float FogDensity = 0.05f;
        float FogHeight = 0.0f;
        float FogHeightFalloff = 1.0f;
        float FogAnisotropy = 0.5f;
        float FogMultiScattering = 0.5f;
        Math::Vec3f FogColor = { 0.7f, 0.8f, 0.9f };
        float FogStartDistance = 1.0f;
    };

} // namespace ecs
} // namespace ge
