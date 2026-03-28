#include "ClothSystem.h"
#include "../World.h"
#include "../components/ClothComponent.h"
#include "../components/TransformComponent.h"
#include "../components/MeshComponent.h"
#include "../components/AnimatorComponent.h"
#include "../../renderer/Mesh.h"
#include "../../debug/log.h"

#include <cmath>
#include <random>

namespace ge {
namespace ecs {

ClothSystem::ClothSystem() {
    signature.set(World::GetComponentType<ClothComponent>());
}

void ClothSystem::Update(World& world, float dt) {
    bool isPlaying = true;
    
    Math::Vec3f gravity = {0.0f, -9.81f, 0.0f};
    
    auto clothEntities = world.Query<ClothComponent, TransformComponent>();
    for (auto entity : clothEntities) {
        auto& cloth = world.GetComponent<ClothComponent>(entity);
        auto& tc = world.GetComponent<TransformComponent>(entity);
        
        if (cloth.Particles.empty()) {
            InitializeCloth(world, entity);
        }
        
        if (cloth.Particles.empty()) continue;
        
        cloth.Time += dt;
        
        ApplyWind(cloth, dt, cloth.Time);
        
        SimulateCloth(cloth, dt, gravity);
        
        SolveConstraints(cloth, (int)cloth.Settings.Iterations);
        
        UpdateNormals(cloth, cloth.Settings.Width, cloth.Settings.Height);
        
        cloth.VerticesDirty = true;
        
        if (world.HasComponent<MeshComponent>(entity)) {
            auto& mc = world.GetComponent<MeshComponent>(entity);
            if (mc.MeshPtr) {
                auto& vertices = mc.MeshPtr->GetVertices();
                
                for (size_t i = 0; i < cloth.Particles.size() && i < vertices.size(); ++i) {
                    vertices[i].Position[0] = cloth.Particles[i].Position.x;
                    vertices[i].Position[1] = cloth.Particles[i].Position.y;
                    vertices[i].Position[2] = cloth.Particles[i].Position.z;
                    
                    vertices[i].Normal[0] = cloth.Particles[i].Normal.x;
                    vertices[i].Normal[1] = cloth.Particles[i].Normal.y;
                    vertices[i].Normal[2] = cloth.Particles[i].Normal.z;
                }
                
                mc.MeshPtr->SetData(vertices.data(), (uint32_t)(vertices.size() * sizeof(renderer::Vertex)));
            }
        }
        
        Math::Vec3f center = {0.0f, 0.0f, 0.0f};
        for (auto& p : cloth.Particles) {
            center += p.Position;
        }
        if (!cloth.Particles.empty()) {
            center /= (float)cloth.Particles.size();
            tc.position = center;
        }
    }
}

void ClothSystem::InitializeCloth(World& world, Entity entity) {
    auto& cloth = world.GetComponent<ClothComponent>(entity);
    auto& tc = world.GetComponent<TransformComponent>(entity);
    
    uint32_t width = cloth.Settings.Width;
    uint32_t height = cloth.Settings.Height;
    float spacing = cloth.Settings.Spacing;
    
    cloth.Particles.resize(width * height);
    cloth.Constraints.clear();
    
    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            uint32_t idx = y * width + x;
            
            float xPos = (float)x * spacing - (float)width * spacing * 0.5f;
            float yPos = 0.0f;
            float zPos = (float)y * spacing - (float)height * spacing * 0.5f;
            
            cloth.Particles[idx].Position = tc.position + Math::Vec3f(xPos, yPos, zPos);
            cloth.Particles[idx].PreviousPosition = cloth.Particles[idx].Position;
            cloth.Particles[idx].Acceleration = {0.0f, 0.0f, 0.0f};
            cloth.Particles[idx].Normal = {0.0f, 0.0f, 1.0f};
            cloth.Particles[idx].InvMass = cloth.Settings.Mass > 0.0f ? 1.0f / (cloth.Settings.Mass / (width * height)) : 0.0f;
            
            if (y == 0 && (x == 0 || x == width - 1)) {
                cloth.Particles[idx].IsPinned = true;
            }
        }
    }
    
    auto addConstraint = [&](int32_t a, int32_t b, float stiffness) {
        if (a >= 0 && b >= 0 && (uint32_t)a < cloth.Particles.size() && (uint32_t)b < cloth.Particles.size()) {
            ClothConstraint constraint;
            constraint.ParticleA = a;
            constraint.ParticleB = b;
            constraint.RestLength = Math::Length(cloth.Particles[a].Position - cloth.Particles[b].Position);
            constraint.Stiffness = stiffness;
            cloth.Constraints.push_back(constraint);
        }
    };
    
    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            uint32_t idx = y * width + x;
            
            if (x < width - 1) {
                addConstraint(idx, idx + 1, cloth.Settings.Stiffness);
            }
            if (y < height - 1) {
                addConstraint(idx, idx + width, cloth.Settings.Stiffness);
            }
            
            if (x < width - 1 && y < height - 1) {
                addConstraint(idx, idx + width + 1, cloth.Settings.Stiffness);
                addConstraint(idx + 1, idx + width, cloth.Settings.Stiffness);
            }
            
            if (x < width - 2) {
                addConstraint(idx, idx + 2, cloth.Settings.Stiffness * 0.5f);
            }
            if (y < height - 2) {
                addConstraint(idx, idx + width * 2, cloth.Settings.Stiffness * 0.5f);
            }
        }
    }
    
    if (world.HasComponent<MeshComponent>(entity)) {
        auto& mc = world.GetComponent<MeshComponent>(entity);
        
        std::vector<renderer::Vertex> vertices;
        std::vector<uint32_t> indices;
        
        for (auto& p : cloth.Particles) {
            renderer::Vertex v;
            v.Position[0] = p.Position.x;
            v.Position[1] = p.Position.y;
            v.Position[2] = p.Position.z;
            v.Normal[0] = 0.0f;
            v.Normal[1] = 0.0f;
            v.Normal[2] = 1.0f;
            v.TexCoord[0] = 0.0f;
            v.TexCoord[1] = 0.0f;
            v.Color[0] = 1.0f;
            v.Color[1] = 1.0f;
            v.Color[2] = 1.0f;
            v.Color[3] = 1.0f;
            vertices.push_back(v);
        }
        
        for (uint32_t y = 0; y < height - 1; ++y) {
            for (uint32_t x = 0; x < width - 1; ++x) {
                uint32_t idx = y * width + x;
                
                indices.push_back(idx);
                indices.push_back(idx + width);
                indices.push_back(idx + 1);
                
                indices.push_back(idx + 1);
                indices.push_back(idx + width);
                indices.push_back(idx + width + 1);
            }
        }
        
        mc.MeshPtr = renderer::Mesh::Create(vertices, indices);
    }
    
    GE_LOG_INFO("[ClothSystem] Initialized cloth with {} particles, {} constraints", 
                cloth.Particles.size(), cloth.Constraints.size());
}

void ClothSystem::SimulateCloth(ClothComponent& cloth, float dt, const Math::Vec3f& gravity) {
    float damping = cloth.Settings.Damping;
    
    for (auto& particle : cloth.Particles) {
        if (particle.IsPinned) continue;
        
        Math::Vec3f velocity = (particle.Position - particle.PreviousPosition) * damping;
        
        particle.PreviousPosition = particle.Position;
        
        particle.Position += velocity + gravity * cloth.Settings.GravityFactor * dt * dt;
    }
}

void ClothSystem::SolveConstraints(ClothComponent& cloth, int iterations) {
    for (int iter = 0; iter < iterations; ++iter) {
        for (auto& constraint : cloth.Constraints) {
            auto& pA = cloth.Particles[constraint.ParticleA];
            auto& pB = cloth.Particles[constraint.ParticleB];
            
            Math::Vec3f delta = pB.Position - pA.Position;
            float currentLength = Math::Length(delta);
            
            if (currentLength < 0.0001f) continue;
            
            float diff = (currentLength - constraint.RestLength) / currentLength;
            Math::Vec3f correction = delta * diff * 0.5f * constraint.Stiffness;
            
            if (!pA.IsPinned && pA.InvMass > 0.0f) {
                pA.Position += correction;
            }
            if (!pB.IsPinned && pB.InvMass > 0.0f) {
                pB.Position -= correction;
            }
        }
    }
}

void ClothSystem::UpdateNormals(ClothComponent& cloth, uint32_t width, uint32_t height) {
    for (auto& particle : cloth.Particles) {
        particle.Normal = {0.0f, 0.0f, 0.0f};
    }
    
    for (uint32_t y = 0; y < height - 1; ++y) {
        for (uint32_t x = 0; x < width - 1; ++x) {
            uint32_t idx = y * width + x;
            
            Math::Vec3f p0 = cloth.Particles[idx].Position;
            Math::Vec3f p1 = cloth.Particles[idx + 1].Position;
            Math::Vec3f p2 = cloth.Particles[idx + width].Position;
            
            Math::Vec3f v1 = p1 - p0;
            Math::Vec3f v2 = p2 - p0;
            Math::Vec3f normal = Math::Cross(v1, v2);
            
            cloth.Particles[idx].Normal += normal;
            cloth.Particles[idx + 1].Normal += normal;
            cloth.Particles[idx + width].Normal += normal;
            
            if (x + 2 < width && y + 1 < height) {
                Math::Vec3f p3 = cloth.Particles[idx + width + 1].Position;
                Math::Vec3f v3 = p3 - p0;
                Math::Vec3f normal2 = Math::Cross(v2, v3);
                cloth.Particles[idx].Normal += normal2;
                cloth.Particles[idx + width].Normal += normal2;
                cloth.Particles[idx + width + 1].Normal += normal2;
            }
        }
    }
    
    for (auto& particle : cloth.Particles) {
        float len = Math::Length(particle.Normal);
        if (len > 0.0001f) {
            particle.Normal /= len;
        } else {
            particle.Normal = {0.0f, 0.0f, 1.0f};
        }
    }
}

void ClothSystem::ApplyWind(ClothComponent& cloth, float dt, float time) {
    if (cloth.Settings.WindFactor <= 0.0f) return;
    
    Math::Vec3f windDir = Math::Normalize(cloth.Settings.WindDirection);
    float turbulence = cloth.Settings.WindTurbulence;
    
    for (size_t i = 0; i < cloth.Particles.size(); ++i) {
        if (cloth.Particles[i].IsPinned) continue;
        
        float noise = sinf(time * 2.0f + (float)i * 0.1f) * turbulence;
        float windStrength = cloth.Settings.WindFactor * (1.0f + noise);
        
        Math::Vec3f wind = windDir * windStrength;
        
        Math::Vec3f normal = cloth.Particles[i].Normal;
        float windEffect = Math::Dot(Math::Normalize(wind), normal);
        windEffect = fabsf(windEffect);
        
        cloth.Particles[i].Position += wind * windEffect * dt * dt;
    }
}

} // namespace ecs
} // namespace ge
