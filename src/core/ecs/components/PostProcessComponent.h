#pragma once

#include "../../math/VecTypes.h"

namespace ge {
namespace ecs {

    struct PostProcessComponent
    {
        bool Enabled = true;

        // Bloom
        bool BloomEnabled = true;
        float BloomIntensity = 1.0f;
        float BloomThreshold = 1.0f;

        // Color Grading / Tonemapping
        float Exposure = 1.0f;
        float Gamma = 2.2f;

        // Volumetric Fog
        bool VolumetricFogEnabled = false;
        float FogDensity = 0.05f;
        float FogHeight = 0.0f;
        float FogHeightFalloff = 1.0f;
        float FogAnisotropy = 0.5f;
        float FogMultiScattering = 0.5f;
        Math::Vec3f FogColor = { 0.7f, 0.8f, 0.9f };
        float FogStartDistance = 1.0f;
        
        // Volumetric Quality
        int VolumetricSamples = 64;
        float VolumetricJitter = 0.5f;

        PostProcessComponent() = default;
        PostProcessComponent(const PostProcessComponent&) = default;
    };

} // namespace ecs
} // namespace ge
