#pragma once

#include <string>
#include <cstdint>
#include <cmath>
#include "../../math/VecTypes.h"
#include "../../audio/AudioCategory.h"

namespace ge {
namespace ecs {

enum class AudioDistanceModel {
    Linear,           // Default: linear falloff
    InverseDistance,  // 1 / (1 + rolloff * (d - ref))
    ExponentialDistance, // pow(d / ref, -rolloff)
    Logarithmic,      // log-based falloff
    CustomCurve      // User-defined curve
};

inline float LinearDistanceAttenuation(float distance, float refDistance, float maxDistance, float rolloff) {
    if (distance <= refDistance) return 1.0f;
    if (distance >= maxDistance) return 0.0f;
    float attenuation = 1.0f - (distance - refDistance) / (maxDistance - refDistance);
    return attenuation * rolloff + (1.0f - rolloff);
}

inline float InverseDistanceAttenuation(float distance, float refDistance, float rolloff) {
    if (distance <= refDistance) return 1.0f;
    return refDistance / (refDistance + rolloff * (distance - refDistance));
}

inline float ExponentialDistanceAttenuation(float distance, float refDistance, float rolloff) {
    if (distance <= refDistance) return 1.0f;
    return std::pow(distance / refDistance, -rolloff);
}

inline float LogarithmicDistanceAttenuation(float distance, float refDistance, float rolloff) {
    if (distance <= refDistance) return 1.0f;
    return 1.0f / (1.0f + rolloff * std::log(distance / refDistance + 1.0f));
}

struct AudioSourceComponent {
    uint64_t ClipHandle = 0;
    std::string FilePath;

    float Volume   = 1.0f;
    float Pitch    = 1.0f;
    bool  Loop     = false;
    bool  PlayOnAwake = false;
    bool  Is3D     = false;

    audio::AudioCategory Category = audio::AudioCategory::SFX;

    float MinDistance = 1.0f;
    float MaxDistance = 100.0f;
    float RolloffFactor = 1.0f;
    float ReferenceDistance = 1.0f;
    AudioDistanceModel DistanceModel = AudioDistanceModel::InverseDistance;

    bool EnableObstruction = true;
    float ObstructionMultiplier = 1.0f;

    bool EnableCone = false;
    float InnerConeAngle = 360.0f;
    float OuterConeAngle = 360.0f;
    float OuterConeVolume = 0.0f;
    Math::Vec3f Direction = Math::Vec3f::Forward();

    bool  IsPlaying   = false;
    bool  HasStarted  = false;
    void* InternalSound = nullptr;

    float CalculateAttenuation(float distance) const {
        switch (DistanceModel) {
            case AudioDistanceModel::Linear:
                return LinearDistanceAttenuation(distance, ReferenceDistance, MaxDistance, RolloffFactor);
            case AudioDistanceModel::InverseDistance:
                return InverseDistanceAttenuation(distance, ReferenceDistance, RolloffFactor);
            case AudioDistanceModel::ExponentialDistance:
                return ExponentialDistanceAttenuation(distance, ReferenceDistance, RolloffFactor);
            case AudioDistanceModel::Logarithmic:
                return LogarithmicDistanceAttenuation(distance, ReferenceDistance, RolloffFactor);
            default:
                return 1.0f;
        }
    }
};

} // namespace ecs
} // namespace ge
