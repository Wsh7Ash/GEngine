#pragma once

#include "core/uuid/UUID.h"
#include "SceneChunk.h"
#include "../ecs/World.h"
#include <string>
#include <vector>
#include <future>
#include <functional>
#include <mutex>
#include <atomic>
#include <queue>

namespace ge {
namespace scene {

struct SerializedEntity {
    UUID id;
    std::string name;
    std::vector<std::pair<std::string, std::string>> components;
    UUID parentId;
    std::vector<UUID> childIds;
};

struct SerializedChunk {
    uint32_t chunkId = 0;
    std::vector<SerializedEntity> entities;
    std::vector<std::string> assetPaths;
    bool isValid = false;
};

struct SerializationResult {
    bool success = false;
    std::string errorMessage;
    float progress = 0.0f;
    size_t serializedEntityCount = 0;
    size_t serializedChunkCount = 0;
};

struct DeserializationResult {
    bool success = false;
    std::string errorMessage;
    float progress = 0.0f;
    SerializedChunk chunkData;
    std::vector<ecs::Entity> createdEntities;
};

using ProgressCallback = std::function<void(float progress, const std::string& status)>;
using CompletionCallback = std::function<void(const SerializationResult& result)>;

class AsyncSerializer {
public:
    explicit AsyncSerializer(ecs::World* world);
    ~AsyncSerializer();
    
    std::future<SerializationResult> SerializeAsync(
        const std::string& filepath,
        ProgressCallback progressCallback = nullptr,
        CompletionCallback completionCallback = nullptr
    );
    
    std::future<DeserializationResult> DeserializeAsync(
        const std::string& filepath,
        ProgressCallback progressCallback = nullptr,
        CompletionCallback completionCallback = nullptr
    );
    
    std::future<SerializationResult> SerializeChunkAsync(
        uint32_t chunkId,
        const std::string& outputPath,
        ProgressCallback progressCallback = nullptr
    );
    
    std::future<DeserializationResult> DeserializeChunkAsync(
        const std::string& inputPath,
        ProgressCallback progressCallback = nullptr
    );
    
    std::future<SerializationResult> SerializeRegionAsync(
        const Math::AABB& bounds,
        const std::string& outputPath,
        ProgressCallback progressCallback = nullptr
    );
    
    void CancelAll();
    bool HasPendingOperations() const;
    
    size_t GetPendingOperationCount() const { return pendingOperations_.load(); }
    
    void SetMaxConcurrency(int maxThreads);
    int GetMaxConcurrency() const { return maxConcurrency_; }
    
private:
    SerializationResult SerializeInternal(const std::string& filepath, ProgressCallback progress);
    DeserializationResult DeserializeInternal(const std::string& filepath, ProgressCallback progress);
    
    SerializationResult SerializeChunkInternal(uint32_t chunkId, const std::string& outputPath, ProgressCallback progress);
    DeserializationResult DeserializeChunkInternal(const std::string& inputPath, ProgressCallback progress);
    
    SerializationResult SerializeRegionInternal(const Math::AABB& bounds, const std::string& outputPath, ProgressCallback progress);
    
    void SerializeEntity(ecs::Entity entity, SerializedEntity& serialized, float& progress);
    ecs::Entity DeserializeEntity(const SerializedEntity& serialized);
    
    ecs::World* world_;
    
    std::atomic<bool> isCancelled_ = false;
    std::atomic<size_t> pendingOperations_ = 0;
    std::atomic<int> activeOperations_ = 0;
    
    int maxConcurrency_ = 4;
    
    std::mutex queueMutex_;
    std::queue<std::function<void()>> pendingQueue_;
    std::condition_variable queueCV_;
    std::vector<std::thread> workerThreads_;
    bool isShutdown_ = false;
    
    std::mutex resultMutex_;
    std::vector<SerializationResult> recentResults_;
    std::vector<DeserializationResult> recentDeserializeResults_;
    
    void WorkerThread();
    void EnqueueTask(std::function<void()> task);
};

class SerializationPool {
public:
    static SerializationPool& Get();
    
    template<typename T>
    std::future<T> Enqueue(std::function<T()> task) {
        std::promise<T> promise;
        std::future<T> future = promise.get_future();
        
        EnqueueTask([task = std::move(task), promise = std::move(promise)]() mutable {
            try {
                T result = task();
                promise.set_value(std::move(result));
            } catch (...) {
                promise.set_exception(std::current_exception());
            }
        });
        
        return future;
    }
    
    void Initialize(int threadCount = 4);
    void Shutdown();
    bool HasPendingWork() const;
    
private:
    SerializationPool() = default;
    ~SerializationPool();
    
    void WorkerLoop();
    void EnqueueTask(std::function<void()> task);
    
    std::queue<std::function<void()>> taskQueue_;
    std::vector<std::thread> workers_;
    std::condition_variable queueCV_;
    std::mutex queueMutex_;
    bool isRunning_ = false;
};

}
}
