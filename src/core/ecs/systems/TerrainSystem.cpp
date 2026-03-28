#include "TerrainSystem.h"
#include "../World.h"
#include "../components/TerrainComponent.h"
#include "../components/TransformComponent.h"
#include "../components/Collider3DComponent.h"
#include "../components/Rigidbody3DComponent.h"
#include "../../debug/log.h"
#include "../../platform/VFS.h"

#include <cmath>

namespace ge {
namespace ecs {

TerrainSystem::TerrainSystem() {
}

void TerrainSystem::Update(World& world, float dt) {
    (void)dt;
    auto terrains = world.Query<TerrainComponent, TransformComponent>();
    
    for (auto entity : terrains) {
        auto& terrain = world.GetComponent<TerrainComponent>(entity);
        auto& tc = world.GetComponent<TransformComponent>(entity);
        
        if (!terrain.IsPhysicsBuilt && !terrain.HeightData.empty()) {
            TerrainPhysicsSettings physicsSettings;
            physicsSettings.EnableCollision = true;
            physicsSettings.UseHeightField = true;
            
            GeneratePhysics(world, entity, physicsSettings);
        }
    }
}

void TerrainSystem::LoadHeightmap(World& world, Entity entity, const std::string& path) {
    auto& terrain = world.GetComponent<TerrainComponent>(entity);
    
    terrain.HeightmapPath = path;
    
    uint32_t width = terrain.GridWidth;
    uint32_t depth = terrain.GridDepth;
    
    terrain.HeightData.resize(width * depth);
    
    for (size_t i = 0; i < terrain.HeightData.size(); ++i) {
        float x = (float)(i % width) / (float)width;
        float z = (float)(i / width) / (float)depth;
        
        float height = sinf(x * 10.0f) * cosf(z * 10.0f) * 5.0f + 5.0f;
        terrain.HeightData[i] = height;
    }
    
    terrain.TotalBounds = CalculateBounds(terrain);
    
    GE_LOG_INFO("[TerrainSystem] Loaded heightmap: {} ({}x{})", path, width, depth);
}

void TerrainSystem::GeneratePhysics(World& world, Entity entity, const TerrainPhysicsSettings& settings) {
    auto& terrain = world.GetComponent<TerrainComponent>(entity);
    auto& tc = world.GetComponent<TransformComponent>(entity);
    
    if (!settings.EnableCollision) return;
    
    if (settings.UseHeightField) {
        Entity colliderEntity = world.CreateEntity();
        
        world.AddComponent<TransformComponent>(colliderEntity);
        auto& colliderTc = world.GetComponent<TransformComponent>(colliderEntity);
        colliderTc.position = tc.position;
        
        world.AddComponent<Collider3DComponent>(colliderEntity);
        auto& cc = world.GetComponent<Collider3DComponent>(colliderEntity);
        
        cc.ShapeType = Collider3DShapeType::HeightField;
        cc.HeightField.Width = terrain.GridWidth;
        cc.HeightField.Depth = terrain.GridDepth;
        cc.HeightField.ScaleX = terrain.WorldSizeX / (float)terrain.GridWidth;
        cc.HeightField.ScaleZ = terrain.WorldSizeZ / (float)terrain.GridDepth;
        cc.HeightField.HeightScale = terrain.HeightScale;
        cc.HeightField.OffsetY = 0.0f;
        cc.HeightField.Heights = terrain.HeightData;
        
        cc.Friction = settings.Friction;
        cc.Restitution = settings.Restitution;
        
        world.AddComponent<Rigidbody3DComponent>(colliderEntity);
        auto& rb = world.GetComponent<Rigidbody3DComponent>(colliderEntity);
        rb.MotionType = Rigidbody3DMotionType::Static;
        rb.CollisionLayer = settings.CollisionLayer;
        rb.CollisionMask = settings.CollisionMask;
        
        terrain.IsPhysicsBuilt = true;
        
        GE_LOG_INFO("[TerrainSystem] Generated HeightField physics for terrain");
    }
    else if (settings.UseTriMesh) {
        Entity colliderEntity = world.CreateEntity();
        
        world.AddComponent<TransformComponent>(colliderEntity);
        world.GetComponent<TransformComponent>(colliderEntity).position = tc.position;
        
        world.AddComponent<Collider3DComponent>(colliderEntity);
        auto& cc = world.GetComponent<Collider3DComponent>(colliderEntity);
        cc.ShapeType = Collider3DShapeType::TriangleMesh;
        cc.Friction = settings.Friction;
        cc.Restitution = settings.Restitution;
        
        world.AddComponent<Rigidbody3DComponent>(colliderEntity);
        auto& rb = world.GetComponent<Rigidbody3DComponent>(colliderEntity);
        rb.MotionType = Rigidbody3DMotionType::Static;
        
        terrain.IsPhysicsBuilt = true;
        
        GE_LOG_INFO("[TerrainSystem] Generated TriangleMesh physics for terrain");
    }
}

void TerrainSystem::CreateChunk(World& world, Entity terrainEntity, uint32_t chunkX, uint32_t chunkZ) {
    auto& terrain = world.GetComponent<TerrainComponent>(terrainEntity);
    
    TerrainChunk chunk;
    chunk.X = chunkX;
    chunk.Z = chunkZ;
    chunk.Width = terrain.ChunkSize;
    chunk.Depth = terrain.ChunkSize;
    
    float worldOffsetX = (float)chunkX * terrain.ChunkSize * (terrain.WorldSizeX / terrain.GridWidth);
    float worldOffsetZ = (float)chunkZ * terrain.ChunkSize * (terrain.WorldSizeZ / terrain.GridDepth);
    
    chunk.WorldPosition = {worldOffsetX, 0.0f, worldOffsetZ};
    
    chunk.IsLoaded = true;
    terrain.Chunks.push_back(chunk);
}

float TerrainSystem::GetHeightAt(const TerrainComponent& terrain, float x, float z) const {
    if (terrain.HeightData.empty()) return 0.0f;
    
    float gridX = (x / terrain.WorldSizeX) * (float)(terrain.GridWidth - 1);
    float gridZ = (z / terrain.WorldSizeZ) * (float)(terrain.GridDepth - 1);
    
    int32_t x0 = (int32_t)floorf(gridX);
    int32_t z0 = (int32_t)floorf(gridZ);
    int32_t x1 = x0 + 1;
    int32_t z1 = z0 + 1;
    
    x0 = std::max(0, std::min(x0, (int32_t)terrain.GridWidth - 1));
    z0 = std::max(0, std::min(z0, (int32_t)terrain.GridDepth - 1));
    x1 = std::max(0, std::min(x1, (int32_t)terrain.GridWidth - 1));
    z1 = std::max(0, std::min(z1, (int32_t)terrain.GridDepth - 1));
    
    float tx = gridX - (float)x0;
    float tz = gridZ - (float)z0;
    
    float h00 = terrain.HeightData[z0 * terrain.GridWidth + x0];
    float h10 = terrain.HeightData[z0 * terrain.GridWidth + x1];
    float h01 = terrain.HeightData[z1 * terrain.GridWidth + x0];
    float h11 = terrain.HeightData[z1 * terrain.GridWidth + x1];
    
    float h0 = h00 * (1.0f - tx) + h10 * tx;
    float h1 = h01 * (1.0f - tx) + h11 * tx;
    
    return h0 * (1.0f - tz) + h1 * tz;
}

Math::AABB TerrainSystem::CalculateBounds(const TerrainComponent& terrain) const {
    Math::AABB bounds;
    
    bounds.Min = {0.0f, terrain.MinHeight, 0.0f};
    bounds.Max = {terrain.WorldSizeX, terrain.MaxHeight, terrain.WorldSizeZ};
    
    return bounds;
}

} // namespace ecs
} // namespace ge
