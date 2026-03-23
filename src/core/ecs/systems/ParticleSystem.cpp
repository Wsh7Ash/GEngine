#include "ParticleSystem.h"
#include "../../ge_core.h"
#include "../components/ParticleEmitterComponent.h"
#include "../../renderer/Renderer2D.h"

namespace ge {
namespace ecs {

ParticleSystem::ParticleSystem()
    : pool_(10000)
{
}

void ParticleSystem::Update(World& world, float dt)
{
    for (auto entity : world.Query<ParticleEmitterComponent, TransformComponent>())
    {
        auto& emitter = world.GetComponent<ParticleEmitterComponent>(entity);
        auto& transform = world.GetComponent<TransformComponent>(entity);

        if (!emitter.IsEmitting) continue;

        // Cache props for pool Update
        activeProps_ = emitter.Props;

        // Accumulate time and emit particles
        emitter.EmissionAccumulator += dt * emitter.EmissionRate;
        while (emitter.EmissionAccumulator >= 1.0f)
        {
            pool_.Emit(emitter.Props, transform.position);
            emitter.EmissionAccumulator -= 1.0f;
        }
    }

    // Tick all active particles
    pool_.Update(dt, activeProps_);
}

void ParticleSystem::Render(const renderer::OrthographicCamera& camera)
{
    renderer::Renderer2D::BeginScene(camera);

    for (auto& p : pool_.GetParticles())
    {
        if (!p.Active) continue;

        Math::Vec2f pos2d = { p.Position.x, p.Position.y };
        Math::Vec2f size2d = { p.Size, p.Size };

        renderer::Renderer2D::DrawQuad(pos2d, size2d, p.Color);
    }

    renderer::Renderer2D::EndScene();
}

} // namespace ecs
} // namespace ge
