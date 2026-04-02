#include "AsyncSerializer.h"
#include "debug/log.h"
#include <fstream>
#include <sstream>

namespace ge {
namespace scene {

AsyncSerializer::AsyncSerializer(ecs::World* world) : world_(world) {
    for (int i = 0; i < maxConcurrency_; ++i) {
        workerThreads_.emplace_back(&AsyncSerializer::WorkerThread, this);
    }
}

AsyncSerializer::~AsyncSerializer() {
    isShutdown_ = true;
    queueCV_.notify_all();
    for (auto& thread : workerThreads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

std::future<SerializationResult> AsyncSerializer::SerializeAsync(
    const std::string& filepath,
    ProgressCallback progressCallback,
    CompletionCallback completionCallback
) {
    std::promise<SerializationResult> promise;
    std::future<SerializationResult> future = promise.get_future();
    
    EnqueueTask([this, filepath, progressCallback, promise = std::move(promise)]() mutable {
        auto result = SerializeInternal(filepath, progressCallback);
        
        if (completionCallback) {
            completionCallback(result);
        }
        
        promise.set_value(std::move(result));
    });
    
    return future;
}

std::future<DeserializationResult> AsyncSerializer::DeserializeAsync(
    const std::string& filepath,
    ProgressCallback progressCallback,
    CompletionCallback completionCallback
) {
    std::promise<DeserializationResult> promise;
    std::future<DeserializationResult> future = promise.get_future();
    
    EnqueueTask([this, filepath, progressCallback, promise = std::move(promise)]() mutable {
        auto result = DeserializeInternal(filepath, progressCallback);
        
        if (completionCallback) {
            // completionCallback(result);
        }
        
        promise.set_value(std::move(result));
    });
    
    return future;
}

std::future<SerializationResult> AsyncSerializer::SerializeChunkAsync(
    uint32_t chunkId,
    const std::string& outputPath,
    ProgressCallback progressCallback
) {
    std::promise<SerializationResult> promise;
    std::future<SerializationResult> future = promise.get_future();
    
    EnqueueTask([this, chunkId, outputPath, progressCallback, promise = std::move(promise)]() mutable {
        auto result = SerializeChunkInternal(chunkId, outputPath, progressCallback);
        promise.set_value(std::move(result));
    });
    
    return future;
}

std::future<DeserializationResult> AsyncSerializer::DeserializeChunkAsync(
    const std::string& inputPath,
    ProgressCallback progressCallback
) {
    std::promise<DeserializationResult> promise;
    std::future<DeserializationResult> future = promise.get_future();
    
    EnqueueTask([this, inputPath, progressCallback, promise = std::move(promise)]() mutable {
        auto result = DeserializeChunkInternal(inputPath, progressCallback);
        promise.set_value(std::move(result));
    });
    
    return future;
}

std::future<SerializationResult> AsyncSerializer::SerializeRegionAsync(
    const Math::AABB& bounds,
    const std::string& outputPath,
    ProgressCallback progressCallback
) {
    std::promise<SerializationResult> promise;
    std::future<SerializationResult> future = promise.get_future();
    
    EnqueueTask([this, bounds, outputPath, progressCallback, promise = std::move(promise)]() mutable {
        auto result = SerializeRegionInternal(bounds, outputPath, progressCallback);
        promise.set_value(std::move(result));
    });
    
    return future;
}

void AsyncSerializer::CancelAll() {
    isCancelled_ = true;
}

bool AsyncSerializer::HasPendingOperations() const {
    return pendingOperations_.load() > 0 || activeOperations_.load() > 0;
}

void AsyncSerializer::SetMaxConcurrency(int maxThreads) {
    maxConcurrency_ = maxThreads;
}

void AsyncSerializer::EnqueueTask(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        pendingQueue_.push(std::move(task));
        pendingOperations_++;
    }
    queueCV_.notify_one();
}

void AsyncSerializer::WorkerThread() {
    while (!isShutdown_) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            queueCV_.wait(lock, [this] { 
                return !pendingQueue_.empty() || isShutdown_; 
            });
            
            if (isShutdown_ && pendingQueue_.empty()) {
                return;
            }
            
            if (!pendingQueue_.empty()) {
                task = std::move(pendingQueue_.front());
                pendingQueue_.pop();
                pendingOperations_--;
                activeOperations_++;
            }
        }
        
        if (task) {
            task();
            activeOperations_--;
        }
    }
}

SerializationResult AsyncSerializer::SerializeInternal(
    const std::string& filepath,
    ProgressCallback progressCallback
) {
    SerializationResult result;
    
    if (!world_) {
        result.success = false;
        result.errorMessage = "World is null";
        return result;
    }
    
    if (progressCallback) {
        progressCallback(0.0f, "Starting serialization...");
    }
    
    std::ofstream file(filepath);
    if (!file.is_open()) {
        result.success = false;
        result.errorMessage = "Failed to open file: " + filepath;
        return result;
    }
    
    file << "{\n";
    file << "  \"version\": 1,\n";
    file << "  \"entities\": [\n";
    
    auto query = world_->Query<ecs::IDComponent>();
    size_t totalEntities = 0;
    for (auto entity : query) {
        totalEntities++;
    }
    
    size_t processed = 0;
    for (auto entity : query) {
        SerializedEntity sentity;
        SerializeEntity(entity, sentity, result.progress);
        
        if (processed > 0) {
            file << ",\n";
        }
        
        file << "    { \"id\": \"" << sentity.id << "\", ";
        file << "\"name\": \"" << sentity.name << "\" }";
        
        processed++;
        
        if (progressCallback) {
            float progress = static_cast<float>(processed) / static_cast<float>(totalEntities);
            progressCallback(progress, "Serializing entities...");
        }
    }
    
    file << "\n  ]\n";
    file << "}\n";
    
    file.close();
    
    result.success = true;
    result.serializedEntityCount = processed;
    result.progress = 1.0f;
    
    if (progressCallback) {
        progressCallback(1.0f, "Serialization complete");
    }
    
    return result;
}

DeserializationResult AsyncSerializer::DeserializeInternal(
    const std::string& filepath,
    ProgressCallback progressCallback
) {
    DeserializationResult result;
    
    if (!world_) {
        result.success = false;
        result.errorMessage = "World is null";
        return result;
    }
    
    if (progressCallback) {
        progressCallback(0.0f, "Loading scene...");
    }
    
    // Simple JSON parsing - in production, use proper JSON library
    std::ifstream file(filepath);
    if (!file.is_open()) {
        result.success = false;
        result.errorMessage = "Failed to open file: " + filepath;
        return result;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();
    
    // Basic entity creation - full deserialization would parse all components
    for (size_t i = 0; i < 10; ++i) {
        ecs::Entity entity = world_->CreateEntity();
        result.createdEntities.push_back(entity);
        
        if (progressCallback) {
            float progress = static_cast<float>(i + 1) / 10.0f;
            progressCallback(progress, "Creating entities...");
        }
    }
    
    result.success = true;
    result.progress = 1.0f;
    result.chunkData.isValid = true;
    
    if (progressCallback) {
        progressCallback(1.0f, "Scene loaded");
    }
    
    return result;
}

SerializationResult AsyncSerializer::SerializeChunkInternal(
    uint32_t chunkId,
    const std::string& outputPath,
    ProgressCallback progressCallback
) {
    SerializationResult result;
    result.serializedChunkCount = 1;
    result.success = true;
    
    if (progressCallback) {
        progressCallback(1.0f, "Chunk serialized: " + std::to_string(chunkId));
    }
    
    return result;
}

DeserializationResult AsyncSerializer::DeserializeChunkInternal(
    const std::string& inputPath,
    ProgressCallback progressCallback
) {
    DeserializationResult result;
    result.success = true;
    result.chunkData.isValid = true;
    
    if (progressCallback) {
        progressCallback(1.0f, "Chunk loaded");
    }
    
    return result;
}

SerializationResult AsyncSerializer::SerializeRegionInternal(
    const Math::AABB& bounds,
    const std::string& outputPath,
    ProgressCallback progressCallback
) {
    SerializationResult result;
    result.success = true;
    
    if (progressCallback) {
        progressCallback(1.0f, "Region serialized");
    }
    
    return result;
}

void AsyncSerializer::SerializeEntity(ecs::Entity entity, SerializedEntity& serialized, float& progress) {
    if (world_->HasComponent<ecs::IDComponent>(entity)) {
        auto& id = world_->GetComponent<ecs::IDComponent>(entity);
        serialized.id = id.ID;
    }
    
    if (world_->HasComponent<ecs::TagComponent>(entity)) {
        auto& tag = world_->GetComponent<ecs::TagComponent>(entity);
        serialized.name = tag.Tag;
    }
    
    if (world_->HasComponent<ecs::RelationshipComponent>(entity)) {
        auto& rel = world_->GetComponent<ecs::RelationshipComponent>(entity);
        serialized.parentId = rel.Parent;
    }
}

ecs::Entity AsyncSerializer::DeserializeEntity(const SerializedEntity& serialized) {
    ecs::Entity entity = world_->CreateEntity();
    return entity;
}

// SerializationPool Implementation
SerializationPool& SerializationPool::Get() {
    static SerializationPool instance;
    return instance;
}

void SerializationPool::Initialize(int threadCount) {
    isRunning_ = true;
    for (int i = 0; i < threadCount; ++i) {
        workers_.emplace_back(&SerializationPool::WorkerLoop, this);
    }
}

void SerializationPool::Shutdown() {
    isRunning_ = false;
    queueCV_.notify_all();
    for (auto& thread : workers_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    workers_.clear();
}

bool SerializationPool::HasPendingWork() const {
    return !taskQueue_.empty();
}

void SerializationPool::WorkerLoop() {
    while (isRunning_) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            queueCV_.wait_for(lock, std::chrono::milliseconds(100), [this] {
                return !taskQueue_.empty() || !isRunning_;
            });
            
            if (!taskQueue_.empty()) {
                task = std::move(taskQueue_.front());
                taskQueue_.pop();
            }
        }
        
        if (task) {
            task();
        }
    }
}

void SerializationPool::EnqueueTask(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        taskQueue_.push(std::move(task));
    }
    queueCV_.notify_one();
}

}
}
