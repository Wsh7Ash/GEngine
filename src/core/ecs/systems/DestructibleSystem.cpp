#include "DestructibleSystem.h"
#include "../World.h"
#include "../components/JointComponent.h"
#include "../components/TransformComponent.h"
#include "../components/MeshComponent.h"
#include "../components/Rigidbody3DComponent.h"
#include "../components/Collider3DComponent.h"
#include "../../renderer/Mesh.h"
#include "../../debug/log.h"

#include <random>
#include <algorithm>

namespace ge {
namespace ecs {

DestructibleSystem::DestructibleSystem() {
    signature.set(World::GetComponentType<DestructibleComponent>());
}

void DestructibleSystem::Update(World& world, float dt) {
    auto destructibles = world.Query<DestructibleComponent, Rigidbody3DComponent>();
    
    for (auto entity : destructibles) {
        auto& dc = world.GetComponent<DestructibleComponent>(entity);
        
        if (dc.IsFractured) continue;
        
        if (!world.HasComponent<Rigidbody3DComponent>(entity)) continue;
        
        auto& rb = world.GetComponent<Rigidbody3DComponent>(entity);
        
        if (rb.MotionType != Rigidbody3DMotionType::Dynamic) continue;
        
        float velocity = 0.0f;
        if (rb.RuntimeBody) {
            JPH::BodyID* bodyID = (JPH::BodyID*)rb.RuntimeBody;
            if (bodyID) {
                velocity = bodyID->GetID();
            }
        }
        
        if (velocity > dc.Settings.FractureThreshold && dc.Settings.Enabled) {
            dc.IsFractured = true;
            
            Math::Vec3f impactPoint = {0.0f, 0.0f, 0.0f};
            if (world.HasComponent<TransformComponent>(entity)) {
                impactPoint = world.GetComponent<TransformComponent>(entity).position;
            }
            
            CreateFragments(world, entity, impactPoint);
            
            if (dc.OnFracture) {
                dc.OnFracture(entity, dc.FragmentEntities);
            }
            
            world.DestroyEntity(entity);
            
            GE_LOG_INFO("[DestructibleSystem] Object fractured into {} fragments", dc.FragmentEntities.size());
        }
    }
}

void DestructibleSystem::CreateFragments(World& world, Entity original, const Math::Vec3f& impactPoint) {
    auto& dc = world.GetComponent<DestructibleComponent>(original);
    
    if (!world.HasComponent<MeshComponent>(original)) return;
    
    auto& originalMesh = world.GetComponent<MeshComponent>(original);
    if (!originalMesh.MeshPtr) return;
    
    const auto& vertices = originalMesh.MeshPtr->GetVertices();
    const auto& indices = originalMesh.MeshPtr->GetIndices();
    
    if (vertices.empty() || indices.empty()) return;
    
    Math::Vec3f originalPos = {0.0f, 0.0f, 0.0f};
    if (world.HasComponent<TransformComponent>(original)) {
        originalPos = world.GetComponent<TransformComponent>(original).position;
    }
    
    int numFragments = dc.Settings.MaxFragments;
    if (dc.Settings.KeepLargestFragment) {
        numFragments = dc.Settings.MaxFragments - 1;
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> velDist(-2.0f, 2.0f);
    std::uniform_real_distribution<float> rotDist(-3.14f, 3.14f);
    
    for (int i = 0; i < numFragments; ++i) {
        Entity fragment = world.CreateEntity();
        
        world.AddComponent<TransformComponent>(fragment);
        auto& tc = world.GetComponent<TransformComponent>(fragment);
        tc.position = originalPos + Math::Vec3f(velDist(gen) * 0.2f, velDist(gen) * 0.2f, velDist(gen) * 0.2f);
        tc.rotation = Math::Quatf(rotDist(gen), rotDist(gen), rotDist(gen), 1.0f);
        tc.scale = Math::Vec3f(0.3f, 0.3f, 0.3f);
        
        std::vector<renderer::Vertex> fragVertices;
        std::vector<uint32_t> fragIndices;
        
        size_t startIdx = (i * vertices.size() / numFragments);
        size_t endIdx = ((i + 1) * vertices.size() / numFragments);
        
        for (size_t j = startIdx; j < endIdx && j < vertices.size(); ++j) {
            fragVertices.push_back(vertices[j]);
        }
        
        std::vector<uint32_t> indexRemap(vertices.size(), UINT32_MAX);
        for (size_t j = startIdx; j < endIdx && j < vertices.size(); ++j) {
            indexRemap[j] = (uint32_t)(j - startIdx);
        }
        
        for (size_t j = 0; j < indices.size(); ++j) {
            uint32_t idx = indices[j];
            if (idx >= startIdx && idx < endIdx) {
                fragIndices.push_back(indexRemap[idx]);
            }
        }
        
        if (fragVertices.size() < 3 || fragIndices.size() < 3) {
            world.DestroyEntity(fragment);
            continue;
        }
        
        world.AddComponent<MeshComponent>(fragment);
        auto& mc = world.GetComponent<MeshComponent>(fragment);
        mc.MeshPtr = renderer::Mesh::Create(fragVertices, fragIndices);
        mc.AlbedoColor = originalMesh.AlbedoColor;
        mc.Metallic = originalMesh.Metallic;
        mc.Roughness = originalMesh.Roughness;
        
        world.AddComponent<Rigidbody3DComponent>(fragment);
        auto& rb = world.GetComponent<Rigidbody3DComponent>(fragment);
        rb.MotionType = Rigidbody3DMotionType::Dynamic;
        rb.Mass = 0.5f;
        rb.Friction = 0.5f;
        rb.Restitution = 0.3f;
        
        world.AddComponent<Collider3DComponent>(fragment);
        auto& cc = world.GetComponent<Collider3DComponent>(fragment);
        cc.ShapeType = Collider3DShapeType::ConvexHull;
        
        auto& fragVel = dc.Settings.FragmentVelocity;
        if (world.HasComponent<VelocityComponent>(fragment)) {
            auto& vel = world.GetComponent<VelocityComponent>(fragment);
            vel.Linear = Math::Vec3f(fragVel.x + velDist(gen), fragVel.y + velDist(gen), fragVel.z + velDist(gen));
            vel.Angular = Math::Vec3f(rotDist(gen), rotDist(gen), rotDist(gen));
        } else {
            world.AddComponent<VelocityComponent>(fragment, VelocityComponent{
                .Linear = Math::Vec3f(fragVel.x + velDist(gen), fragVel.y + velDist(gen), fragVel.z + velDist(gen)),
                .Angular = Math::Vec3f(rotDist(gen), rotDist(gen), rotDist(gen))
            });
        }
        
        dc.FragmentEntities.push_back(fragment);
    }
    
    if (dc.Settings.KeepLargestFragment) {
        Entity largest = world.CreateEntity();
        
        world.AddComponent<TransformComponent>(largest);
        auto& tc = world.GetComponent<TransformComponent>(largest);
        tc.position = originalPos;
        
        world.AddComponent<MeshComponent>(largest);
        auto& mc = world.GetComponent<MeshComponent>(largest);
        mc.MeshPtr = originalMesh.MeshPtr;
        mc.AlbedoColor = originalMesh.AlbedoColor;
        mc.Metallic = originalMesh.Metallic;
        mc.Roughness = originalMesh.Roughness;
        
        world.AddComponent<Rigidbody3DComponent>(largest);
        auto& rb = world.GetComponent<Rigidbody3DComponent>(largest);
        rb.MotionType = Rigidbody3DMotionType::Dynamic;
        rb.Mass = 1.0f;
        
        world.AddComponent<Collider3DComponent>(largest);
        
        dc.FragmentEntities.push_back(largest);
    }
    
    if (dc.Settings.GenerateDebris) {
        int numDebris = (int)(dc.Settings.MaxFragments * dc.Settings.DebrisThreshold);
        
        for (int i = 0; i < numDebris; ++i) {
            Entity debris = world.CreateEntity();
            
            world.AddComponent<TransformComponent>(debris);
            auto& tc = world.GetComponent<TransformComponent>(debris);
            tc.position = originalPos + Math::Vec3f(velDist(gen) * 0.5f, velDist(gen) * 0.5f, velDist(gen) * 0.5f);
            tc.scale = Math::Vec3f(0.05f, 0.05f, 0.05f);
            
            auto cube = renderer::Mesh::CreateCube();
            world.AddComponent<MeshComponent>(debris);
            auto& mc = world.GetComponent<MeshComponent>(debris);
            mc.MeshPtr = cube;
            mc.AlbedoColor = Math::Vec3f(0.3f, 0.3f, 0.3f);
            
            world.AddComponent<Rigidbody3DComponent>(debris);
            auto& rb = world.GetComponent<Rigidbody3DComponent>(debris);
            rb.MotionType = Rigidbody3DMotionType::Dynamic;
            rb.Mass = 0.1f;
            
            world.AddComponent<Collider3DComponent>(debris);
            auto& cc = world.GetComponent<Collider3DComponent>(debris);
            cc.ShapeType = Collider3DShapeType::Box;
            cc.BoxHalfExtents = Math::Vec3f(0.025f, 0.025f, 0.025f);
            
            dc.FragmentEntities.push_back(debris);
        }
    }
}

} // namespace ecs
} // namespace ge
