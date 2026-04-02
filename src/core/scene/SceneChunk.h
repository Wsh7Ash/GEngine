#pragma once

#include "../uuid/UUID.h"
#include "../math/VecTypes.h"
#include "../math/BoundingVolumes.h"
#include <string>
#include <vector>
#include <atomic>
#include <memory>

namespace ge {
namespace scene {

enum class ChunkState {
    Unloaded = 0,
    Loading,
    Loaded,
    Unloading,
    Failed
};

struct SceneChunk {
    uint32_t chunkId = 0;
    Math::AABB bounds;
    Math::Vec3i gridPosition = {0, 0, 0};
    
    std::vector<UUID> entityIds;
    std::vector<std::string> assetPaths;
    
    ChunkState state = ChunkState::Unloaded;
    float loadProgress = 0.0f;
    
    std::atomic<int> referenceCount{0};
    std::atomic<bool> isValid{false};
    
    std::string filePath;
    std::string name;
    
    float lastAccessTime = 0.0f;
    float priority = 0.0f;
    
    SceneChunk() = default;
    
    SceneChunk(uint32_t id, const Math::Vec3i& gridPos, const Math::AABB& bounds)
        : chunkId(id), gridPosition(gridPos), bounds(bounds) {
        name = "Chunk_" + std::to_string(id);
    }
    
    bool IsLoaded() const { return state == ChunkState::Loaded; }
    bool IsLoading() const { return state == ChunkState::Loading; }
    bool IsUnloaded() const { return state == ChunkState::Unloaded; }
    
    void AddEntity(UUID entityId) {
        entityIds.push_back(entityId);
    }
    
    void RemoveEntity(UUID entityId) {
        for (auto it = entityIds.begin(); it != entityIds.end(); ++it) {
            if (*it == entityId) {
                entityIds.erase(it);
                return;
            }
        }
    }
    
    bool HasEntity(UUID entityId) const {
        for (const auto& id : entityIds) {
            if (id == entityId) return true;
        }
        return false;
    }
    
    void AddDependency(const std::string& assetPath) {
        for (const auto& path : assetPaths) {
            if (path == assetPath) return;
        }
        assetPaths.push_back(assetPath);
    }
    
    void IncRef() { ++referenceCount; }
    void DecRef() { --referenceCount; }
    int GetRef() const { return referenceCount.load(); }
};

struct SceneChunkManifest {
    uint32_t version = 1;
    std::string sceneName;
    Math::Vec3i gridSize = {4, 1, 4};
    Math::Vec3f chunkSize = {64.0f, 64.0f, 64.0f};
    Math::Vec3f worldOffset = {0.0f, 0.0f, 0.0f};
    uint32_t chunkCount = 0;
    std::vector<SceneChunk> chunks;
    
    static const uint32_t CURRENT_VERSION = 1;
    
    bool IsValid() const { return version == CURRENT_VERSION; }
    
    SceneChunk* FindChunk(uint32_t chunkId) {
        for (auto& chunk : chunks) {
            if (chunk.chunkId == chunkId) return &chunk;
        }
        return nullptr;
    }
    
    const SceneChunk* FindChunk(uint32_t chunkId) const {
        for (const auto& chunk : chunks) {
            if (chunk.chunkId == chunkId) return &chunk;
        }
        return nullptr;
    }
    
    SceneChunk* FindChunkAtPosition(const Math::Vec3f& worldPos) {
        Math::Vec3i gridPos = WorldToGrid(worldPos);
        return FindChunkByGridPosition(gridPos);
    }
    
    SceneChunk* FindChunkByGridPosition(const Math::Vec3i& gridPos) {
        for (auto& chunk : chunks) {
            if (chunk.gridPosition == gridPos) return &chunk;
        }
        return nullptr;
    }
    
    Math::Vec3i WorldToGrid(const Math::Vec3f& worldPos) const {
        int x = static_cast<int>(std::floor((worldPos.x - worldOffset.x) / chunkSize.x));
        int y = static_cast<int>(std::floor((worldPos.y - worldOffset.y) / chunkSize.y));
        int z = static_cast<int>(std::floor((worldPos.z - worldOffset.z) / chunkSize.z));
        return {x, y, z};
    }
    
    Math::Vec3f GridToWorld(const Math::Vec3i& gridPos) const {
        return {
            worldOffset.x + gridPos.x * chunkSize.x,
            worldOffset.y + gridPos.y * chunkSize.y,
            worldOffset.z + gridPos.z * chunkSize.z
        };
    }
};

class SceneChunkManager {
public:
    SceneChunkManager() = default;
    ~SceneChunkManager() = default;
    
    void Initialize(const SceneChunkManifest& manifest);
    void Shutdown();
    
    SceneChunk* GetChunk(uint32_t chunkId);
    const SceneChunk* GetChunk(uint32_t chunkId) const;
    
    std::vector<SceneChunk*> GetChunksInRadius(const Math::Vec3f& center, float radius);
    std::vector<SceneChunk*> GetChunksInBox(const Math::AABB& box);
    
    uint32_t GetLoadedChunkCount() const;
    uint32_t GetLoadingChunkCount() const;
    
    void SetChunkState(uint32_t chunkId, ChunkState state);
    void SetChunkProgress(uint32_t chunkId, float progress);
    
    const SceneChunkManifest& GetManifest() const { return manifest_; }
    SceneChunkManifest& GetManifest() { return manifest_; }
    
    void MarkChunkValid(uint32_t chunkId, bool valid);
    
private:
    SceneChunkManifest manifest_;
};

}
}
