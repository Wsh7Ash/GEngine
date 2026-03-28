#pragma once

#include "../System.h"
#include "../../math/VecTypes.h"
#include "../components/BuoyancyComponent.h"
#include "../components/WaterVolumeComponent.h"

namespace ge {
namespace ecs {

class BuoyancySystem : public System {
public:
    BuoyancySystem();
    ~BuoyancySystem() = default;

    void Update(World& world, float dt);

private:
    float GetWaterHeightAt(const WaterVolumeComponent& water, const Math::Vec3f& position, float time);
    float CalculateSubmersion(World& world, Entity entity, const WaterVolumeComponent& water, float time);
    void ApplyBuoyancyForce(World& world, Entity entity, BuoyancyComponent& buoyancy, Rigidbody3DComponent& rb, float submergedVolume, float dt);
    void ApplyDragForce(World& world, Entity entity, BuoyancyComponent& buoyancy, Rigidbody3DComponent& rb, const Math::Vec3f& velocity, float dt);
    void ApplyFlowForce(World& world, Entity entity, BuoyancyComponent& buoyancy, Rigidbody3DComponent& rb, const WaterVolumeComponent& water, float dt);
    Math::Vec3f GetVolumeCenterOfMass(World& world, Entity entity);
    Math::Vec3f GetVelocity(World& world, Entity entity);
};

} // namespace ecs
} // namespace ge
