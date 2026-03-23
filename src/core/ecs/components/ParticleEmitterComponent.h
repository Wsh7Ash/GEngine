#pragma once

#include "../../particles/Particle.h"

namespace ge {
namespace ecs {

struct ParticleEmitterComponent {
    particles::ParticleProps Props;

    float EmissionRate       = 10.0f;   // Particles per second
    bool  IsEmitting         = true;

    // Runtime (not serialized)
    float EmissionAccumulator = 0.0f;
};

} // namespace ecs
} // namespace ge
