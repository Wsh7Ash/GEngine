#pragma once

#include "../System.h"
#include "../../particles/ParticlePool.h"
#include "../../renderer/OrthographicCamera.h"

namespace ge {
namespace ecs {

/**
 * @brief System that drives particle emission, simulation, and rendering.
 */
class ParticleSystem : public System {
public:
    ParticleSystem();

    void Update(World& world, float dt);
    void Render(const renderer::OrthographicCamera& camera);

private:
    particles::ParticlePool pool_;
    particles::ParticleProps activeProps_; // cached for Update in pool
};

} // namespace ecs
} // namespace ge
