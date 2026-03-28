#include "BuoyancySystem.h"
#include "../World.h"
#include "../components/BuoyancyComponent.h"
#include "../components/WaterVolumeComponent.h"
#include "../components/TransformComponent.h"
#include "../components/Rigidbody3DComponent.h"
#include "../components/Collider3DComponent.h"
#include "../components/VelocityComponent.h"
#include "../../debug/log.h"

#include <cmath>

namespace ge {
namespace ecs {

static const Math::Vec3f GRAVITY = {0.0f, -9.81f, 0.0f};

BuoyancySystem::BuoyancySystem() {
}

void BuoyancySystem::Update(World& world, float dt) {
    auto waterVolumes = world.Query<WaterVolumeComponent, TransformComponent>();
    auto buoyantBodies = world.Query<BuoyancyComponent, Rigidbody3DComponent, TransformComponent>();
    
    bool hasWater = false;
    for (auto e : waterVolumes) {
        (void)e;
        hasWater = true;
        break;
    }
    
    bool hasBodies = false;
    for (auto e : buoyantBodies) {
        (void)e;
        hasBodies = true;
        break;
    }
    
    if (!hasWater || !hasBodies) {
        return;
    }
    
    float totalTime = 0.0f;
    
    for (auto waterEntity : waterVolumes) {
        auto& water = world.GetComponent<WaterVolumeComponent>(waterEntity);
        auto& waterTc = world.GetComponent<TransformComponent>(waterEntity);
        
        if (!water.IsActive) continue;
        
        for (auto entity : buoyantBodies) {
            auto& buoyancy = world.GetComponent<BuoyancyComponent>(entity);
            auto& rb = world.GetComponent<Rigidbody3DComponent>(entity);
            auto& tc = world.GetComponent<TransformComponent>(entity);
            
            if (rb.MotionType != Rigidbody3DMotionType::Dynamic) continue;
            
            Math::Vec3f centerOfMass = GetVolumeCenterOfMass(world, entity);
            
            float waterHeight = GetWaterHeightAt(water, centerOfMass, totalTime);
            
            float submersion = CalculateSubmersion(world, entity, water, totalTime);
            
            buoyancy.SubmergedAmount = Math::Clamp(submersion, 0.0f, 1.0f);
            buoyancy.IsSubmerged = submersion > 0.0f;
            
            if (buoyancy.IsSubmerged) {
                ApplyBuoyancyForce(world, entity, buoyancy, rb, submersion, dt);
                ApplyDragForce(world, entity, buoyancy, rb, GetVelocity(world, entity), dt);
                ApplyFlowForce(world, entity, buoyancy, rb, water, dt);
            }
        }
    }
}

float BuoyancySystem::GetWaterHeightAt(const WaterVolumeComponent& water, const Math::Vec3f& position, float time) {
    float waveHeight = 0.0f;
    
    if (water.WaveAmplitude > 0.0f) {
        float phase = water.WaveFrequency * position.x + water.WaveFrequency * position.z + time * water.WaveSpeed;
        waveHeight = water.WaveAmplitude * sinf(phase);
    }
    
    return water.WaterHeight + waveHeight;
}

float BuoyancySystem::CalculateSubmersion(World& world, Entity entity, const WaterVolumeComponent& water, float time) {
    auto& buoyancy = world.GetComponent<BuoyancyComponent>(entity);
    auto& rb = world.GetComponent<Rigidbody3DComponent>(entity);
    auto& tc = world.GetComponent<TransformComponent>(entity);
    
    float totalSubmersion = 0.0f;
    int sampleCount = 0;
    
    if (buoyancy.Mode == BuoyancyMode::Simple) {
        Math::Vec3f centerPos = tc.position + buoyancy.Offset;
        float waterHeight = GetWaterHeightAt(water, centerPos, time);
        
        float depth = waterHeight - centerPos.y;
        
        if (depth > 0.0f) {
            float colliderHeight = 1.0f;
            if (world.HasComponent<Collider3DComponent>(entity)) {
                auto& cc = world.GetComponent<Collider3DComponent>(entity);
                colliderHeight = cc.BoxHalfExtents.y * 2.0f;
            }
            
            totalSubmersion = Math::Clamp(depth / colliderHeight, 0.0f, 1.0f);
        }
    }
    else if (buoyancy.Mode == BuoyancyMode::Volumetric) {
        int numSamples = buoyancy.SamplePoints;
        float step = 1.0f / (float)(numSamples - 1);
        
        Math::Vec3f aabbMin = tc.position - Math::Vec3f(0.5f, 0.5f, 0.5f);
        Math::Vec3f aabbMax = tc.position + Math::Vec3f(0.5f, 0.5f, 0.5f);
        
        if (world.HasComponent<Collider3DComponent>(entity)) {
            auto& cc = world.GetComponent<Collider3DComponent>(entity);
            aabbMin = tc.position - cc.BoxHalfExtents;
            aabbMax = tc.position + cc.BoxHalfExtents;
        }
        
        for (int x = 0; x < numSamples; ++x) {
            for (int y = 0; y < numSamples; ++y) {
                for (int z = 0; z < numSamples; ++z) {
                    float t_x = (float)x * step;
                    float t_y = (float)y * step;
                    float t_z = (float)z * step;
                    Math::Vec3f samplePos = {
                        aabbMin.x + (aabbMax.x - aabbMin.x) * t_x,
                        aabbMin.y + (aabbMax.y - aabbMin.y) * t_y,
                        aabbMin.z + (aabbMax.z - aabbMin.z) * t_z
                    };
                    float waterHeight = GetWaterHeightAt(water, samplePos, time);
                    
                    float depth = waterHeight - samplePos.y;
                    if (depth > 0.0f) {
                        totalSubmersion += depth;
                    }
                    sampleCount++;
                }
            }
        }
        
        if (sampleCount > 0) {
            float avgDepth = totalSubmersion / sampleCount;
            float maxDepth = aabbMax.y - aabbMin.y;
            totalSubmersion = Math::Clamp(avgDepth / maxDepth, 0.0f, 1.0f);
        }
    }
    else if (buoyancy.Mode == BuoyancyMode::Linear) {
        Math::Vec3f centerPos = tc.position + buoyancy.Offset;
        float waterHeight = GetWaterHeightAt(water, centerPos, time);
        
        float colliderHeight = 1.0f;
        if (world.HasComponent<Collider3DComponent>(entity)) {
            auto& cc = world.GetComponent<Collider3DComponent>(entity);
            colliderHeight = cc.BoxHalfExtents.y * 2.0f;
        }
        
        float bottomY = centerPos.y - colliderHeight * 0.5f;
        float topY = centerPos.y + colliderHeight * 0.5f;
        
        if (topY < waterHeight && bottomY > waterHeight) {
            totalSubmersion = 1.0f;
        } else if (topY > waterHeight && bottomY < waterHeight) {
            totalSubmersion = (topY - waterHeight) / colliderHeight;
        } else if (topY <= waterHeight) {
            totalSubmersion = 1.0f;
        }
    }
    
    return totalSubmersion;
}

void BuoyancySystem::ApplyBuoyancyForce(World& world, Entity entity, BuoyancyComponent& buoyancy, Rigidbody3DComponent& rb, float submergedVolume, float dt) {
    float displacedVolume = rb.Mass * submergedVolume;
    float buoyancyForceMagnitude = buoyancy.FluidDensity * displacedVolume * (-GRAVITY.y);
    
    buoyancy.BuoyancyForce = buoyancyForceMagnitude;
    
    Math::Vec3f buoyancyForce = {0.0f, buoyancyForceMagnitude, 0.0f};
    
    if (!world.HasComponent<VelocityComponent>(entity)) {
        world.AddComponent<VelocityComponent>(entity, VelocityComponent{});
    }
    auto& vel = world.GetComponent<VelocityComponent>(entity);
    vel.velocity += buoyancyForce * dt;
}

void BuoyancySystem::ApplyDragForce(World& world, Entity entity, BuoyancyComponent& buoyancy, Rigidbody3DComponent& rb, const Math::Vec3f& velocity, float dt) {
    (void)rb;
    if (!world.HasComponent<VelocityComponent>(entity)) {
        world.AddComponent<VelocityComponent>(entity, VelocityComponent{});
    }
    auto& vel = world.GetComponent<VelocityComponent>(entity);
    
    Math::Vec3f dragForce = -velocity * buoyancy.LinearDrag;
    vel.velocity += dragForce * dt;
}

void BuoyancySystem::ApplyFlowForce(World& world, Entity entity, BuoyancyComponent& buoyancy, Rigidbody3DComponent& rb, const WaterVolumeComponent& water, float dt) {
    (void)rb;
    if (water.FlowSpeed > 0.0f && buoyancy.FlowStrength > 0.0f) {
        Math::Vec3f flowDir = Math::Normalize(water.FlowDirection);
        Math::Vec3f flowForce = flowDir * water.FlowSpeed * buoyancy.FlowStrength;
        
        if (!world.HasComponent<VelocityComponent>(entity)) {
            world.AddComponent<VelocityComponent>(entity, VelocityComponent{});
        }
        auto& vel = world.GetComponent<VelocityComponent>(entity);
        vel.velocity += flowForce * dt;
    }
}

Math::Vec3f BuoyancySystem::GetVolumeCenterOfMass(World& world, Entity entity) {
    auto& tc = world.GetComponent<TransformComponent>(entity);
    return tc.position;
}

Math::Vec3f BuoyancySystem::GetVelocity(World& world, Entity entity) {
    if (world.HasComponent<VelocityComponent>(entity)) {
        return world.GetComponent<VelocityComponent>(entity).velocity;
    }
    return Math::Vec3f(0.0f, 0.0f, 0.0f);
}

} // namespace ecs
} // namespace ge
