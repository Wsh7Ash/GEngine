#pragma once

#include "SceneChunk.h"
#include "AsyncSerializer.h"
#include "../ecs/World.h"
#include <string>
#include <memory>
#include <vector>
#include <atomic>
#include <future>
#include <mutex>

namespace ge {
namespace scene {

enum class LoadFlags {
    None = 0,
    Async = 1 << 0,
    Background = 1 << 1,
    PreloadDependencies = 1 << 2,
    EnableProgressCallback = 1 << 3
};

inline LoadFlags operator|(LoadFlags a, LoadFlags b) {
    return static_cast<LoadFlags>(static_cast<int>(a) | static_cast<int>(b));
}

inline LoadFlags operator&(LoadFlags a, LoadFlags b) {
    return static_cast<LoadFlags>(static_cast<int>(a) & static_cast<int>(b));
}

struct StreamingSettings {
    float streamingRadius = 128.0f;
    float unloadRadius = 256.0f;
    int maxLoadedChunks = 32;
    int maxConcurrentLoads = 4;
    float priorityScale = 1.0f;
    bool enablePreloading = true;
    float preloadAheadDistance = 64.0f;
    float fadeTransitionTime = 0.5f;
};

struct LoadingProgress {
    float overallProgress = 0.0f;
    float chunkProgress = 0.0f;
    size_t loadedChunks = 0;
    size_t totalChunks = 0;
    size_t loadedAssets = 0;
    size_t totalAssets = 0;
    std::string currentOperation;
    std::string currentChunk;
    bool isLoading = false;
    bool isComplete = false;
    std::string errorMessage;
    
    LoadingProgress() = default;
    
    float GetChunkProgress() const {
        if (totalChunks == 0) return 0.0f;
        return static_cast<float>(loadedChunks) / static_cast<float>(totalChunks);
    }
    
    float GetAssetProgress() const {
        if (totalAssets == 0) return 0.0f;
        return static_cast<float>(loadedAssets) / static_cast<float>(totalAssets);
    }
};

class SceneManager {
public:
    static SceneManager& Get();
    
    explicit SceneManager(ecs::World* world);
    ~SceneManager();
    
    void Initialize();
    void Shutdown();
    
    std::future<bool> LoadSceneAsync(
        const std::string& scenePath,
        LoadFlags flags = LoadFlags::Async | LoadFlags::EnableProgressCallback,
        ProgressCallback progressCallback = nullptr
    );
    
    std::future<bool> LoadChunkAsync(
        uint32_t chunkId,
        ProgressCallback progressCallback = nullptr
    );
    
    std::future<bool> UnloadChunkAsync(
        uint32_t chunkId,
        ProgressCallback progressCallback = nullptr
    );
    
    bool LoadScene(
        const std::string& scenePath,
        LoadFlags flags = LoadFlags::None
    );
    
    bool LoadChunk(uint32_t chunkId);
    bool UnloadChunk(uint32_t chunkId);
    
    void SaveScene(const std::string& scenePath);
    void SaveChunk(uint32_t chunkId, const std::string& outputPath);
    
    void Update();
    void UpdateStreaming(const Math::Vec3f& focusPoint);
    
    void SetStreamingEnabled(bool enabled);
    bool IsStreamingEnabled() const { return streamingEnabled_; }
    
    void SetStreamingRadius(float radius);
    float GetStreamingRadius() const { return settings_.streamingRadius; }
    
    void SetFocusPoint(const Math::Vec3f& point);
    Math::Vec3f GetFocusPoint() const { return focusPoint_; }
    
    const LoadingProgress& GetLoadingProgress() const { return loadingProgress_; }
    LoadingProgress& GetLoadingProgress() { return loadingProgress_; }
    
    void SetStreamingSettings(const StreamingSettings& settings);
    const StreamingSettings& GetStreamingSettings() const { return settings_; }
    StreamingSettings& GetStreamingSettings() { return settings_; }
    
    bool IsLoading() const { return loadingProgress_.isLoading; }
    bool HasActiveChunks() const;
    
    std::vector<uint32_t> GetLoadedChunkIds() const;
    std::vector<uint32_t> GetLoadingChunkIds() const;
    
    const SceneChunk* GetChunk(uint32_t chunkId) const;
    SceneChunk* GetChunk(uint32_t chunkId);
    
    bool HasChunk(uint32_t chunkId) const;
    
    void PreloadChunksAroundPoint(const Math::Vec3f& point, float radius);
    
    void CancelLoading();
    void WaitForLoading();
    
    ecs::World* GetWorld() { return world_; }
    const ecs::World* GetWorld() const { return world_; }
    
    void SetDefaultScenePath(const std::string& path);
    const std::string& GetDefaultScenePath() const { return defaultScenePath_; }
    
    void SetManifest(const SceneChunkManifest& manifest);
    const SceneChunkManifest& GetManifest() const { return chunkManager_.GetManifest(); }
    
private:
    bool LoadSceneInternal(const std::string& scenePath);
    bool LoadChunkInternal(uint32_t chunkId);
    bool UnloadChunkInternal(uint32_t chunkId);
    
    void UpdateChunkPriorities();
    void UpdateLoadingQueue();
    void ProcessChunkQueue();
    
    ecs::World* world_ = nullptr;
    std::unique_ptr<AsyncSerializer> serializer_;
    SceneChunkManager chunkManager_;
    
    StreamingSettings settings_;
    Math::Vec3f focusPoint_ = {0, 0, 0};
    
    bool streamingEnabled_ = false;
    bool isInitialized_ = false;
    
    LoadingProgress loadingProgress_;
    mutable std::mutex progressMutex_;
    
    std::string defaultScenePath_;
    std::string currentScenePath_;
    
    std::vector<uint32_t> loadQueue_;
    std::vector<uint32_t> unloadQueue_;
    mutable std::mutex queueMutex_;
    
    std::atomic<bool> isLoading_ = false;
    std::atomic<bool> shouldCancel_ = false;
    
    std::mutex loadMutex_;
    std::condition_variable loadCV_;
    
    std::vector<std::future<bool>> pendingLoads_;
    
    std::vector<UUID> loadedEntityIds_;
    std::vector<UUID> pendingEntityIds_;
    
    void BuildChunkManifest(const std::string& scenePath);
    void CreateDefaultChunks();
    
    uint32_t GenerateChunkId(const Math::Vec3i& gridPos) const;
    Math::Vec3i GridPositionFromChunkId(uint32_t chunkId) const;
    
    float CalculateChunkPriority(uint32_t chunkId, const Math::Vec3f& focus) const;
    
    std::vector<SceneChunk*> GetChunksToLoad();
    std::vector<SceneChunk*> GetChunksToUnload();
};

}
}
