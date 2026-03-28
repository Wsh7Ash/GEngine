#pragma once

#include "../../math/VecTypes.h"
#include "../../math/BoundingVolumes.h"
#include <vector>
#include <string>

namespace ge {
namespace ecs {

struct TerrainChunk {
    uint32_t X = 0;
    uint32_t Z = 0;
    uint32_t Width = 0;
    uint32_t Depth = 0;
    Math::Vec3f WorldPosition = {0.0f, 0.0f, 0.0f};
    Math::AABB Bounds;
    bool IsLoaded = false;
};

enum class TerrainLOD {
    Low,
    Medium,
    High
};

struct TerrainComponent {
    uint32_t GridWidth = 256;
    uint32_t GridDepth = 256;
    
    float WorldSizeX = 1000.0f;
    float WorldSizeZ = 1000.0f;
    
    float HeightScale = 100.0f;
    
    std::vector<float> HeightData;
    
    Math::AABB TotalBounds;
    
    std::vector<TerrainChunk> Chunks;
    
    uint32_t ChunkSize = 64;
    uint32_t LODLevels = 3;
    
    TerrainLOD CurrentLOD = TerrainLOD::High;
    
    std::string HeightmapPath = "";
    std::string MaterialPath = "";
    
    float MinHeight = 0.0f;
    float MaxHeight = 100.0f;
    
    bool CastShadows = true;
    bool ReceiveShadows = true;
    
    bool IsNavigationBuilt = false;
    bool IsPhysicsBuilt = false;
};

struct TerrainPhysicsSettings {
    bool EnableCollision = true;
    bool EnableRaycast = true;
    
    float Friction = 0.8f;
    float Restitution = 0.1f;
    
    int CollisionLayer = 1;
    int CollisionMask = 0xFFFF;
    
    bool UseHeightField = true;
    bool UseTriMesh = false;
    
    float CellSize = 1.0f;
};

} // namespace ecs
} // namespace ge
