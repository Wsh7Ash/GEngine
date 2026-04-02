#pragma once

// ================================================================
//  AssetLoader.h
//  Async asset loading with priority queue.
// ================================================================

#include "Asset.h"
#include "AssetCache.h"
#include <memory>
#include <functional>
#include <queue>
#include <atomic>
#include <thread>
#include <future>

namespace ge {
namespace assets {

enum class LoadPriority {
    Critical = 0,
    High = 1,
    Normal = 2,
    Low = 3,
    Prefetch = 4
};

struct LoadRequest {
    AssetHandle handle;
    std::string path;
    LoadPriority priority;
    std::promise<std::shared_ptr<Asset>> promise;
    std::shared_ptr<std::atomic<bool>> cancelled;
    
    bool operator<(const LoadRequest& other) const {
        return priority > other.priority;
    }
};

class AssetLoader {
public:
    static AssetLoader& Get();
    
    AssetLoader();
    ~AssetLoader();
    
    void Initialize(int maxConcurrentLoads = 4);
    void Shutdown();
    
    std::future<std::shared_ptr<Asset>> LoadAsync(AssetHandle handle, const std::string& path, LoadPriority priority = LoadPriority::Normal);
    
    void CancelRequest(AssetHandle handle);
    void CancelAllRequests();
    
    void SetPriority(AssetHandle handle, LoadPriority priority);
    
    bool IsLoading(AssetHandle handle) const;
    bool IsQueued(AssetHandle handle) const;
    
    size_t GetPendingCount() const;
    size_t GetActiveCount() const;
    
    void Update();

private:
    void WorkerThread();
    void ProcessRequest(LoadRequest& request);
    
    std::priority_queue<LoadRequest> requestQueue_;
    std::vector<std::thread> workerThreads_;
    
    std::atomic<bool> shutdown_ = false;
    std::atomic<size_t> activeLoads_ = 0;
    int maxConcurrentLoads_ = 4;
    
    std::mutex queueMutex_;
    std::condition_variable queueCV_;
    
    std::unordered_map<AssetHandle, std::weak_ptr<std::atomic<bool>>> activeRequests_;
    
    AssetCache* cache_;
};

class StreamingAssetLoader {
public:
    StreamingAssetLoader();
    ~StreamingAssetLoader();
    
    void Initialize();
    void Shutdown();
    
    void RequestAsset(AssetHandle handle, const std::string& path, LoadPriority priority = LoadPriority::Normal);
    void CancelRequest(AssetHandle handle);
    
    std::shared_ptr<Asset> GetAsset(AssetHandle handle);
    
    bool IsLoaded(AssetHandle handle) const;
    bool IsLoading(AssetHandle handle) const;
    
    void SetStreamingEnabled(bool enabled) { streamingEnabled_ = enabled; }
    bool IsStreamingEnabled() const { return streamingEnabled_; }
    
    void Update();
    
    void PreloadArea(const Math::Vec3f& center, float radius);
    void UnloadArea(const Math::Vec3f& center, float radius);
    
    void SetMemoryBudget(size_t bytes);
    size_t GetMemoryBudget() const { return memoryBudget_; }

private:
    std::unique_ptr<AssetLoader> loader_;
    AssetCache cache_;
    
    bool streamingEnabled_ = true;
    size_t memoryBudget_ = 512 * 1024 * 1024;
    
    std::atomic<uint64_t> nextRequestId_{0};
};

} // namespace assets
} // namespace ge
