#pragma once

#include "../System.h"

namespace ge {
namespace ecs {

/**
 * @brief System responsible for audio playback and 3D spatialization.
 * Manages AudioSourceComponent entities via MiniAudio.
 */
class AudioSystem : public System {
public:
    AudioSystem() = default;

    void Init();
    void Update(World& world, float dt);
    void Shutdown();
};

} // namespace ecs
} // namespace ge
