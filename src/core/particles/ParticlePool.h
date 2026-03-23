#pragma once

#include "Particle.h"
#include <vector>
#include <cstdint>
#include <random>

namespace ge {
namespace particles {

/**
 * @brief Fixed-size ring-buffer pool of particles.
 * Avoids per-frame allocation. Emit() activates the next slot.
 */
class ParticlePool {
public:
    explicit ParticlePool(uint32_t maxParticles = 10000)
        : pool_(maxParticles), insertIndex_(0),
          rng_(std::random_device{}()) {}

    /// Emit a single particle at the given world position.
    void Emit(const ParticleProps& props, const Math::Vec3f& origin)
    {
        Particle& p = pool_[insertIndex_];
        p.Active        = true;
        p.Position      = origin;
        p.LifeTotal     = props.LifeTime;
        p.LifeRemaining = props.LifeTime;
        p.Rotation      = 0.0f;
        p.Size          = props.SizeStart;

        // Randomized velocity within [Min, Max]
        p.Velocity.x = RandomRange(props.VelocityMin.x, props.VelocityMax.x);
        p.Velocity.y = RandomRange(props.VelocityMin.y, props.VelocityMax.y);
        p.Velocity.z = RandomRange(props.VelocityMin.z, props.VelocityMax.z);

        p.Color = props.ColorStart;

        insertIndex_ = (insertIndex_ + 1) % static_cast<uint32_t>(pool_.size());
    }

    /// Tick all active particles. Lerp size/color over lifetime.
    void Update(float dt, const ParticleProps& defaultProps)
    {
        for (auto& p : pool_)
        {
            if (!p.Active) continue;

            p.LifeRemaining -= dt;
            if (p.LifeRemaining <= 0.0f) {
                p.Active = false;
                continue;
            }

            // Normalized lifetime progress (0 = just born, 1 = about to die)
            float t = 1.0f - (p.LifeRemaining / p.LifeTotal);

            // Physics
            p.Velocity.y -= defaultProps.GravityScale * 9.81f * dt;
            p.Position   = p.Position + p.Velocity * dt;
            p.Rotation  += defaultProps.RotationSpeed * dt;

            // Property curves (linear lerp)
            p.Size = Math::Lerp(defaultProps.SizeStart, defaultProps.SizeEnd, t);

            p.Color.x = Math::Lerp(defaultProps.ColorStart.x, defaultProps.ColorEnd.x, t);
            p.Color.y = Math::Lerp(defaultProps.ColorStart.y, defaultProps.ColorEnd.y, t);
            p.Color.z = Math::Lerp(defaultProps.ColorStart.z, defaultProps.ColorEnd.z, t);
            p.Color.w = Math::Lerp(defaultProps.ColorStart.w, defaultProps.ColorEnd.w, t);
        }
    }

    /// Access the full pool for rendering (check Active flag).
    const std::vector<Particle>& GetParticles() const { return pool_; }

    uint32_t GetActiveCount() const {
        uint32_t count = 0;
        for (auto& p : pool_) if (p.Active) ++count;
        return count;
    }

private:
    float RandomRange(float min, float max) {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(rng_);
    }

    std::vector<Particle> pool_;
    uint32_t insertIndex_;
    std::mt19937 rng_;
};

} // namespace particles
} // namespace ge
