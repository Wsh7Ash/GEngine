#pragma once

#include "../../math/VecTypes.h"
#include "../../math/quaternion.h"
#include <string>
#include <vector>

namespace ge {
namespace ecs {

struct ClothParticle {
    Math::Vec3f Position;
    Math::Vec3f PreviousPosition;
    Math::Vec3f Acceleration;
    Math::Vec3f Normal;
    float InvMass = 1.0f;
    bool IsPinned = false;
    int32_t FixedToBone = -1;
};

struct ClothConstraint {
    int32_t ParticleA = -1;
    int32_t ParticleB = -1;
    float RestLength = 0.0f;
    float Stiffness = 1.0f;
};

struct ClothAttachmentPoint {
    int32_t ParticleIndex = -1;
    Math::Vec3f LocalOffset = {0.0f, 0.0f, 0.0f};
    bool IsAttached = true;
};

enum class ClothRenderMode {
    DoubleSided,
    FrontFacing,
    BackFacing
};

struct ClothCreationSettings {
    uint32_t Width = 10;
    uint32_t Height = 10;
    float Spacing = 0.1f;
    
    float Mass = 1.0f;
    float Damping = 0.99f;
    float Stiffness = 1.0f;
    float Iterations = 3;
    
    float GravityFactor = 1.0f;
    float WindFactor = 0.0f;
    Math::Vec3f WindDirection = {0.0f, 0.0f, 0.0f};
    float WindTurbulence = 0.0f;
    
    bool SelfCollision = false;
    float SelfCollisionRadius = 0.01f;
    
    ClothRenderMode RenderMode = ClothRenderMode::DoubleSided;
};

struct ClothComponent {
    ClothCreationSettings Settings;
    
    std::vector<ClothParticle> Particles;
    std::vector<ClothConstraint> Constraints;
    std::vector<ClothAttachmentPoint> AttachmentPoints;
    
    Math::Vec3f AlbedoColor = {1.0f, 1.0f, 1.0f};
    float Metallic = 0.0f;
    float Roughness = 0.8f;
    
    std::string AlbedoPath = "";
    std::string NormalPath = "";
    std::string AOPath = "";
    
    bool IsVisible = true;
    bool VerticesDirty = false;
    
    float Time = 0.0f;
};

} // namespace ecs
} // namespace ge
