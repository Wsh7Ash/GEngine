#pragma once

#include "../../math/VecTypes.h"
#include "../../math/quaternion.h"
#include <vector>

namespace ge {
namespace ecs {

struct WheelSettings {
    Math::Vec3f Position = {0.0f, 0.0f, 0.0f};
    
    float Radius = 0.35f;
    float Width = 0.25f;
    
    float SuspensionLength = 0.3f;
    float SuspensionStiffness = 30.0f;
    float DampingCompression = 4.0f;
    float DampingRelaxation = 5.0f;
    
    float FrictionSlip = 2.0f;
    float RollInfluence = 0.1f;
    
    bool IsPowered = false;
    bool IsSteeringWheel = false;
    
    int32_t BoneIndex = -1;
    std::string BoneName = "";
    
    float SuspensionForce = 0.0f;
    float SteeringAngle = 0.0f;
    float RotationAngle = 0.0f;
    float WheelRadius = 0.0f;
    
    float CurrentSlip = 0.0f;
    float CurrentCompression = 0.0f;
    
    Entity WheelEntity = INVALID_ENTITY;
};

struct EngineSettings {
    float MaxRPM = 6000.0f;
    float MinRPM = 800.0f;
    
    float PeakPower = 200.0f;
    float PeakTorque = 350.0f;
    
    float Compression = 9.0f;
    
    float EngineBrake = 1.5f;
    
    float TorqueMultiplier = 1.0f;
};

enum class TransmissionType {
    Automatic,
    Manual,
    Sequential
};

struct TransmissionSettings {
    TransmissionType Type = TransmissionType::Automatic;
    
    int ForwardGears = 6;
    int ReverseGears = 1;
    
    float GearRatios[8] = {3.5f, 2.5f, 1.8f, 1.4f, 1.1f, 0.9f, 0.0f, 0.0f};
    float FinalDriveRatio = 3.7f;
    
    float ShiftUpRPM = 5500.0f;
    float ShiftDownRPM = 1500.0f;
    
    float ClutchEngageTime = 0.3f;
    float ClutchReleaseTime = 0.2f;
    
    bool UseAutoClutch = true;
};

struct BrakeSettings {
    float MaxBrakeForce = 1500.0f;
    float MaxParkingBrakeForce = 3000.0f;
    
    float ABSEnabled = true;
    float ABSThreshold = 2.0f;
};

struct VehicleComponent {
    EngineSettings Engine;
    TransmissionSettings Transmission;
    BrakeSettings Brakes;
    
    std::vector<WheelSettings> Wheels;
    
    float ChassisMass = 1500.0f;
    Math::Vec3f ChassisOffset = {0.0f, 0.5f, 0.0f};
    
    Math::Vec3f CenterOfMass = {0.0f, 0.0f, 0.0f};
    float LowerCenterOfMass = 0.5f;
    
    float MaxSteeringAngle = 0.5f;
    float SteeringSpeed = 3.0f;
    
    float AirDrag = 0.3f;
    float RollingResistance = 12.8f;
    
    float BrakeForce = 0.0f;
    float ParkingBrakeForce = 0.0f;
    
    bool IsEngineRunning = false;
    float CurrentRPM = 0.0f;
    float CurrentTorque = 0.0f;
    int CurrentGear = 0;
    float ClutchPedal = 0.0f;
    
    Math::Vec3f Velocity = {0.0f, 0.0f, 0.0f};
    float AngularVelocity = 0.0f;
    
    Math::Vec3f Acceleration = {0.0f, 0.0f, 0.0f};
    float PitchAngle = 0.0f;
    float RollAngle = 0.0f;
    
    bool IsGrounded = false;
    float CurrentSpeed = 0.0f;
    
    void* RuntimeVehicle = nullptr;
};

} // namespace ecs
} // namespace ge
