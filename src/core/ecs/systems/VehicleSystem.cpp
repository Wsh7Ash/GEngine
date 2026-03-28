#include "VehicleSystem.h"
#include "../World.h"
#include "../components/VehicleComponent.h"
#include "../components/TransformComponent.h"
#include "../components/Rigidbody3DComponent.h"
#include "../components/Collider3DComponent.h"
#include "../components/MeshComponent.h"
#include "../components/InputStateComponent.h"
#include "../components/VelocityComponent.h"
#include "../components/NativeScriptComponent.h"
#include "../../debug/log.h"

#include <cmath>

namespace ge {
namespace ecs {

VehicleSystem::VehicleSystem() {
}

void VehicleSystem::Update(World& world, float dt) {
    auto vehicles = world.Query<VehicleComponent, TransformComponent>();
    
    for (auto entity : vehicles) {
        auto& vehicle = world.GetComponent<VehicleComponent>(entity);
        auto& tc = world.GetComponent<TransformComponent>(entity);
        
        if (!vehicle.Wheels.empty()) {
            if (!vehicle.RuntimeVehicle) {
                InitializeVehicle(world, entity);
            }
            
            UpdateEngine(vehicle, dt);
            UpdateTransmission(vehicle, dt);
            UpdateSuspension(vehicle, dt);
            UpdateWheels(vehicle, dt, tc.position, tc.rotation);
            ApplyForces(vehicle, dt);
            SyncWheelVisuals(world, entity);
        }
    }
}

void VehicleSystem::InitializeVehicle(World& world, Entity entity) {
    auto& vehicle = world.GetComponent<VehicleComponent>(entity);
    
    if (!world.HasComponent<Rigidbody3DComponent>(entity)) {
        world.AddComponent<Rigidbody3DComponent>(entity, Rigidbody3DComponent{});
    }
    
    auto& rb = world.GetComponent<Rigidbody3DComponent>(entity);
    rb.MotionType = Rigidbody3DMotionType::Dynamic;
    rb.Mass = vehicle.ChassisMass;
    rb.Friction = 0.5f;
    rb.Restitution = 0.1f;
    rb.LinearDamping = vehicle.AirDrag;
    rb.AngularDamping = 1.0f;
    
    if (!world.HasComponent<Collider3DComponent>(entity)) {
        world.AddComponent<Collider3DComponent>(entity, Collider3DComponent{});
    }
    
    auto& cc = world.GetComponent<Collider3DComponent>(entity);
    cc.ShapeType = Collider3DShapeType::Box;
    cc.BoxHalfExtents = Math::Vec3f(2.0f, 0.5f, 4.0f);
    
    vehicle.CurrentRPM = vehicle.Engine.MinRPM;
    vehicle.IsEngineRunning = true;
    vehicle.CurrentGear = 0;
    
    GE_LOG_INFO("[VehicleSystem] Initialized vehicle with {} wheels", vehicle.Wheels.size());
}

void VehicleSystem::UpdateSuspension(VehicleComponent& vehicle, float dt) {
    vehicle.IsGrounded = false;
    
    for (auto& wheel : vehicle.Wheels) {
        wheel.SuspensionForce = 0.0f;
        wheel.CurrentCompression = 0.0f;
        wheel.CurrentSlip = 0.0f;
        
        float rayLength = wheel.SuspensionLength + wheel.Radius;
        
        float suspensionCompression = 0.0f;
        bool isGrounded = false;
        
        float springForce = 0.0f;
        float damperForce = 0.0f;
        
        float groundHeight = -10.0f;
        float terrainHeight = groundHeight + rayLength;
        float compression = terrainHeight;
        
        if (compression > 0.0f) {
            isGrounded = true;
            suspensionCompression = std::min(compression, wheel.SuspensionLength);
            wheel.CurrentCompression = suspensionCompression / wheel.SuspensionLength;
            
            springForce = wheel.SuspensionStiffness * suspensionCompression;
            
            float wheelVelocity = vehicle.Velocity.y;
            float contactVelocity = wheelVelocity;
            float relativeVelocity = contactVelocity - wheelVelocity;
            
            if (relativeVelocity < 0.0f) {
                damperForce = -wheel.DampingCompression * relativeVelocity;
            } else {
                damperForce = -wheel.DampingRelaxation * relativeVelocity;
            }
            
            wheel.CurrentSlip = Math::Abs(relativeVelocity) * 0.1f;
            wheel.CurrentSlip = Math::Clamp(wheel.CurrentSlip, 0.0f, 1.0f);
        }
        
        wheel.SuspensionForce = springForce + damperForce;
        
        if (isGrounded) {
            vehicle.IsGrounded = true;
        }
    }
}

void VehicleSystem::UpdateEngine(VehicleComponent& vehicle, float dt) {
    if (!vehicle.IsEngineRunning) {
        vehicle.CurrentRPM = 0.0f;
        vehicle.CurrentTorque = 0.0f;
        return;
    }
    
    float gearRatio = CalculateGearRatio(vehicle, vehicle.CurrentGear);
    
    if (vehicle.CurrentSpeed > 0.1f || vehicle.CurrentGear > 0) {
        float speedFactor = vehicle.CurrentSpeed * gearRatio * vehicle.Transmission.FinalDriveRatio * 60.0f / (2.0f * 3.14159f);
        vehicle.CurrentRPM = Math::Clamp(speedFactor, vehicle.Engine.MinRPM, vehicle.Engine.MaxRPM);
    } else {
        vehicle.CurrentRPM = vehicle.Engine.MinRPM;
    }
    
    vehicle.CurrentTorque = CalculateEngineTorque(vehicle, vehicle.CurrentRPM);
    
    if (vehicle.Engine.Compression > 0.0f) {
        vehicle.CurrentRPM -= vehicle.Engine.EngineBrake * vehicle.Engine.Compression * dt;
        vehicle.CurrentRPM = std::max(vehicle.CurrentRPM, vehicle.Engine.MinRPM);
    }
}

float VehicleSystem::CalculateEngineTorque(VehicleComponent& vehicle, float rpm) {
    float normalizedRPM = (rpm - vehicle.Engine.MinRPM) / (vehicle.Engine.MaxRPM - vehicle.Engine.MinRPM);
    normalizedRPM = Math::Clamp(normalizedRPM, 0.0f, 1.0f);
    
    float torque = vehicle.Engine.PeakTorque * (1.0f - vehicle.Engine.Compression * 0.03f * (1.0f - normalizedRPM));
    torque *= vehicle.Engine.TorqueMultiplier;
    
    return std::max(torque, 0.0f);
}

void VehicleSystem::UpdateTransmission(VehicleComponent& vehicle, float dt) {
    if (vehicle.Transmission.Type == TransmissionType::Automatic) {
        UpdateAutoGear(vehicle, dt);
    }
    
    if (vehicle.CurrentGear != 0) {
        vehicle.ClutchPedal = Math::Clamp(vehicle.ClutchPedal - dt * 3.0f, 0.0f, 1.0f);
    } else {
        vehicle.ClutchPedal = Math::Clamp(vehicle.ClutchPedal + dt * 3.0f, 0.0f, 1.0f);
    }
}

void VehicleSystem::UpdateAutoGear(VehicleComponent& vehicle, float dt) {
    (void)dt;
    if (vehicle.CurrentSpeed < 0.5f && vehicle.CurrentGear == 0) {
        return;
    }
    
    if (vehicle.CurrentRPM > vehicle.Transmission.ShiftUpRPM && vehicle.CurrentGear < vehicle.Transmission.ForwardGears) {
        vehicle.CurrentGear++;
    } else if (vehicle.CurrentRPM < vehicle.Transmission.ShiftDownRPM && vehicle.CurrentGear > 1) {
        vehicle.CurrentGear--;
    }
}

float VehicleSystem::CalculateGearRatio(VehicleComponent& vehicle, int gear) {
    if (gear < 0) {
        return -vehicle.Transmission.GearRatios[vehicle.Transmission.ForwardGears + 1] * vehicle.Transmission.FinalDriveRatio;
    } else if (gear == 0) {
        return 0.0f;
    } else if ((size_t)gear < vehicle.Transmission.ForwardGears) {
        return vehicle.Transmission.GearRatios[gear - 1] * vehicle.Transmission.FinalDriveRatio;
    }
    return 0.0f;
}

void VehicleSystem::UpdateWheels(VehicleComponent& vehicle, float dt, const Math::Vec3f& chassisPos, const Math::Quatf& chassisRot) {
    (void)chassisPos;
    (void)chassisRot;
    for (auto& wheel : vehicle.Wheels) {
        if (wheel.IsSteeringWheel) {
            float targetSteer = vehicle.MaxSteeringAngle;
            wheel.SteeringAngle = Math::Lerp(wheel.SteeringAngle, targetSteer, dt * vehicle.SteeringSpeed);
        }
        
        if (vehicle.CurrentGear != 0 && wheel.IsPowered) {
            float wheelAngularVelocity = vehicle.CurrentSpeed / wheel.Radius;
            wheel.RotationAngle += wheelAngularVelocity * dt;
        }
    }
}

void VehicleSystem::ApplyForces(VehicleComponent& vehicle, float dt) {
    vehicle.Acceleration = Math::Vec3f(0.0f, 0.0f, 0.0f);
    
    if (vehicle.CurrentGear != 0 && !vehicle.Wheels.empty()) {
        float gearRatio = CalculateGearRatio(vehicle, vehicle.CurrentGear);
        float totalTorque = vehicle.CurrentTorque * gearRatio * vehicle.ClutchPedal;
        float driveForce = totalTorque / vehicle.Wheels[0].Radius;
        
        vehicle.Acceleration.z = driveForce / vehicle.ChassisMass * dt;
    }
    
    float speed = std::sqrt(vehicle.Velocity.x * vehicle.Velocity.x + vehicle.Velocity.y * vehicle.Velocity.y + vehicle.Velocity.z * vehicle.Velocity.z);
    if (speed > 0.1f) {
        Math::Vec3f dragForce = -vehicle.Velocity * vehicle.AirDrag * speed;
        vehicle.Acceleration += dragForce / vehicle.ChassisMass;
        
        float rollResistance = vehicle.RollingResistance * vehicle.ChassisMass * 9.81f;
        Math::Vec3f normalizedVel = vehicle.Velocity / speed;
        Math::Vec3f rollForce = -normalizedVel * rollResistance;
        vehicle.Acceleration += rollForce / vehicle.ChassisMass;
    }
    
    if (vehicle.Brakes.MaxBrakeForce > 0.0f) {
        float brakeDecel = vehicle.Brakes.MaxBrakeForce / vehicle.ChassisMass;
        float currentSpeed = std::sqrt(vehicle.Velocity.x * vehicle.Velocity.x + vehicle.Velocity.y * vehicle.Velocity.y + vehicle.Velocity.z * vehicle.Velocity.z);
        if (currentSpeed > brakeDecel * dt) {
            Math::Vec3f normalizedVel = vehicle.Velocity / currentSpeed;
            vehicle.Velocity -= normalizedVel * brakeDecel * dt;
        } else {
            vehicle.Velocity = Math::Vec3f(0.0f, 0.0f, 0.0f);
        }
    }
    
    vehicle.CurrentSpeed = std::sqrt(vehicle.Velocity.x * vehicle.Velocity.x + vehicle.Velocity.y * vehicle.Velocity.y + vehicle.Velocity.z * vehicle.Velocity.z);
}

void VehicleSystem::SyncWheelVisuals(World& world, Entity entity) {
    auto& vehicle = world.GetComponent<VehicleComponent>(entity);
    auto& tc = world.GetComponent<TransformComponent>(entity);
    
    for (size_t i = 0; i < vehicle.Wheels.size(); ++i) {
        auto& wheel = vehicle.Wheels[i];
        
        if (wheel.WheelEntity != INVALID_ENTITY && world.IsAlive(wheel.WheelEntity)) {
            auto& wheelTc = world.GetComponent<TransformComponent>(wheel.WheelEntity);
            
            Math::Vec3f wheelOffset = wheel.Position;
            Math::Vec3f worldOffset = tc.rotation * wheelOffset;
            wheelTc.position = tc.position + worldOffset;
            
            Math::Quatf steerQuat = Math::Quatf::FromAxisAngle(Math::Vec3f(0.0f, 1.0f, 0.0f), wheel.SteeringAngle);
            Math::Quatf rollQuat = Math::Quatf::FromAxisAngle(Math::Vec3f(1.0f, 0.0f, 0.0f), wheel.RotationAngle);
            
            wheelTc.rotation = tc.rotation * steerQuat * rollQuat;
        }
    }
}

} // namespace ecs
} // namespace ge
