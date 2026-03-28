#pragma once

#include "../../math/VecTypes.h"
#include "../../math/quaternion.h"
#include <string>
#include <functional>

namespace ge {
namespace ecs {

enum class JointType {
    Hinge,
    Point,
    Fixed,
    Slider,
    Distance,
    Spring
};

enum class JointAxis {
    X,
    Y,
    Z
};

struct JointLimits {
    float Min = 0.0f;
    float Max = 0.0f;
    float ContactDistance = 0.0f;
};

struct JointMotor {
    bool Enabled = false;
    float TargetVelocity = 0.0f;
    float MaxForce = 0.0f;
};

struct JointSpringSettings {
    bool Enabled = false;
    float Frequency = 2.0f;
    float Damping = 0.5f;
};

struct JointCreationSettings {
    JointType Type = JointType::Point;
    
    Entity BodyA;
    Entity BodyB;
    
    Math::Vec3f LocalAnchorA = {0.0f, 0.0f, 0.0f};
    Math::Vec3f LocalAnchorB = {0.0f, 0.0f, 0.0f};
    
    JointAxis AxisA = JointAxis::Y;
    JointAxis AxisB = JointAxis::Y;
    
    JointLimits PositionLimits;
    JointLimits RotationLimits;
    
    JointMotor Motor;
    
    JointSpringSettings LinearSpring;
    JointSpringSettings AngularSpring;
    
    float BreakForce = 0.0f;
    float BreakTorque = 0.0f;
    
    bool EnableCollision = false;
};

struct JointComponent {
    JointCreationSettings Settings;
    
    void* RuntimeJoint = nullptr;
    
    bool IsBroken = false;
    float CurrentStress = 0.0f;
    
    std::function<void(Entity, Entity)> OnBreak;
    std::function<void(Entity, float)> OnStressChanged;
};

struct DestructibleSettings {
    bool Enabled = false;
    
    float FractureThreshold = 1000.0f;
    
    int MaxFragments = 8;
    
    Math::Vec3f FragmentVelocity = {0.0f, 5.0f, 0.0f};
    
    bool KeepLargestFragment = true;
    
    bool GenerateDebris = true;
    float DebrisThreshold = 0.2f;
    
    float ImpulseMultiplier = 1.0f;
};

struct DestructibleComponent {
    DestructibleSettings Settings;
    
    bool IsFractured = false;
    
    std::vector<Entity> FragmentEntities;
    
    std::function<void(Entity, const std::vector<Entity>&)> OnFracture;
};

} // namespace ecs
} // namespace ge
