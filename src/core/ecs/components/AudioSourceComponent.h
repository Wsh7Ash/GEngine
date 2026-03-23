#pragma once

#include <string>
#include <cstdint>

namespace ge {
namespace ecs {

struct AudioSourceComponent {
    uint64_t ClipHandle = 0;       // AssetHandle to audio file
    std::string FilePath;          // Direct file path (fallback)

    float Volume   = 1.0f;
    float Pitch    = 1.0f;
    bool  Loop     = false;
    bool  PlayOnAwake = false;
    bool  Is3D     = false;

    float MinDistance = 1.0f;
    float MaxDistance = 100.0f;

    // Runtime state (not serialized)
    bool  IsPlaying   = false;
    bool  HasStarted  = false;      // Tracks if PlayOnAwake has fired
    void* InternalSound = nullptr;  // Opaque handle to ma_sound
};

} // namespace ecs
} // namespace ge
