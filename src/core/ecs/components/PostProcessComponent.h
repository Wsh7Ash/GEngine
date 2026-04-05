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

        // Tonemapping
        enum class ToneMapping {
            Reinhard = 0,
            ACES = 1,
            Filmic = 2,
            Uncharted2 = 3
        };
        
        int ToneMappingType = 1; // 0=Reinhard, 1=ACES (default), 2=Filmic, 3=Uncharted2
        float WhitePoint = 4.0f;
        
        // Exposure
        float Exposure = 1.0f;
        bool AutoExposure = false;
        float AutoExposureSpeed = 1.0f;
        float AutoExposureMin = 0.03f;
        float AutoExposureMax = 10.0f;

        // Color Grading / Tonemapping
        float Gamma = 2.2f;

        // Vignette
        bool VignetteEnabled = true;
        float VignetteIntensity = 0.4f;
        float VignetteSmoothness = 0.5f;

        // SSAO
        bool SSAOEnabled = true;
        float SSAOIntensity = 1.0f;
        float SSAORadius = 0.5f;
        float SSAOBias = 0.025f;
        int SSAOKernelSize = 64;

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
