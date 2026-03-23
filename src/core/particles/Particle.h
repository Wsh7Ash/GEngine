#pragma once

#include "../math/VecTypes.h"

namespace ge {
namespace particles {

/// Runtime state of a single particle.
struct Particle {
    Math::Vec3f Position    = Math::Vec3f::Zero();
    Math::Vec3f Velocity    = Math::Vec3f::Zero();
    Math::Vec4f Color       = Math::Vec4f::White();
    float       Size        = 1.0f;
    float       Rotation    = 0.0f;
    float       LifeRemaining = 0.0f;
    float       LifeTotal     = 1.0f;
    bool        Active      = false;
};

/// Configuration for how particles are emitted.
struct ParticleProps {
    Math::Vec3f VelocityMin  = { -0.5f, 0.5f, 0.0f };
    Math::Vec3f VelocityMax  = {  0.5f, 1.5f, 0.0f };

    Math::Vec4f ColorStart   = { 1.0f, 0.8f, 0.2f, 1.0f };  // warm yellow
    Math::Vec4f ColorEnd     = { 1.0f, 0.0f, 0.0f, 0.0f };  // fade-out red

    float SizeStart          = 0.15f;
    float SizeEnd            = 0.0f;

    float LifeTime           = 1.5f;
    float RotationSpeed      = 0.0f;
    float GravityScale       = 0.0f;
};

} // namespace particles
} // namespace ge
