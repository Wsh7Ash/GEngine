#pragma once

#include "../System.h"
#include "../../math/VecTypes.h"
#include "../../math/quaternion.h"
#include "components/VehicleComponent.h"

namespace ge {
namespace ecs {

class VehicleSystem : public System {
public:
    VehicleSystem();
    ~VehicleSystem() = default;

    void Update(World& world, float dt);

private:
    void InitializeVehicle(World& world, Entity entity);
    void UpdateSuspension(VehicleComponent& vehicle, float dt);
    void UpdateEngine(VehicleComponent& vehicle, float dt);
    void UpdateTransmission(VehicleComponent& vehicle, float dt);
    void UpdateWheels(VehicleComponent& vehicle, float dt, const Math::Vec3f& chassisPos, const Math::Quatf& chassisRot);
    void ApplyForces(VehicleComponent& vehicle, float dt);
    void SyncWheelVisuals(World& world, Entity entity);
    
    float CalculateEngineTorque(VehicleComponent& vehicle, float rpm);
    float CalculateGearRatio(VehicleComponent& vehicle, int gear);
    void UpdateAutoGear(VehicleComponent& vehicle, float dt);
};

} // namespace ecs
} // namespace ge
