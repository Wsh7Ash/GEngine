#pragma once

#include "../../math/VecTypes.h"
#include "../../math/quaternion.h"
#include <string>
#include <vector>

namespace JPH {
    class SoftBody;
    class SoftBodySharedSettings;
}

namespace ge {
namespace ecs {

enum class SoftBodyMotionType {
    Static = 0,
    Dynamic
};

enum class SoftBodyType {
    Cloth,
    Rope,
    Jello
};

struct SoftBodyCreationSettings {
    float Mass = 1.0f;
    float Stiffness = 0.5f;
    float Damping = 0.05f;
    
    float Pressure = 0.0f;
    
    SoftBodyMotionType MotionType = SoftBodyMotionType::Dynamic;
    SoftBodyType BodyType = SoftBodyType::Jello;
    
    bool FixedVertices = false;
    
    float CorrectionFactor = 0.1f;
    float MaxCorrectionVelocity = 100.0f;
    
    int Iterations = 4;
    
    float GravityFactor = 1.0f;
    float WindFactor = 0.0f;
    Math::Vec3f WindDirection = {0.0f, 0.0f, 0.0f};
    
    uint32_t VertexCount = 0;
    uint32_t FaceCount = 0;
};

struct SoftBodyComponent {
    SoftBodyCreationSettings Settings;
    
    std::string MeshPath = "";
    
    bool IsVisible = true;
    
    Math::Vec3f AlbedoColor = {1.0f, 1.0f, 1.0f};
    float Metallic = 0.0f;
    float Roughness = 0.5f;
    
    std::string AlbedoPath = "";
    std::string NormalPath = "";
    
    void* RuntimeSoftBody = nullptr;
    
    bool VerticesDirty = false;
};

} // namespace ecs
} // namespace ge
