#pragma once

// ================================================================
//  TelemetryUploader.h
//  HTTP upload of telemetry data and crash reports.
// ================================================================

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <thread>
#include <atomic>
#include <chrono>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace ge {
namespace debug {
    class TelemetrySession;
    enum class ExportFormat;
}
}

namespace ge {
namespace telemetry {

struct UploadTask {
    enum class Type {
        TelemetrySession,
        CrashReport,
        MetricsBatch
    };
    
    Type type;
    std::string endpoint;
    std::vector<uint8_t> data;
    int priority = 0;
    int retryCount = 0;
    int maxRetries = 3;
    std::chrono::steady_clock::time_point createdAt;
    
    UploadTask() : createdAt(std::chrono::steady_clock::now()) {}
};

class TelemetryUploader {
public:
    static TelemetryUploader& Get();
    
    void Initialize();
    void Shutdown();
    
    void SetEndpoint(const std::string& endpoint);
    std::string GetEndpoint() const { return endpoint_; }
    
    void SetEnabled(bool enabled);
    bool IsEnabled() const { return enabled_; }
    
    void SetMaxQueueSize(size_t maxSize);
    size_t GetQueueSize() const;
    
    void SetMaxConcurrentUploads(int max);
    int GetMaxConcurrentUploads() const { return maxConcurrentUploads_; }
    
    void SetUploadInterval(std::chrono::seconds interval);
    std::chrono::seconds GetUploadInterval() const { return uploadInterval_; }
    
    bool QueueSessionUpload(debug::TelemetrySession* session);
    bool QueueCrashUpload(const std::string& crashPath);
    bool QueueMetricsUpload(const std::vector<uint8_t>& metricsData);
    
    void SetUploadCallback(std::function<void(bool, const std::string&)> callback);
    
    struct UploadStats {
        size_t totalUploads = 0;
        size_t successfulUploads = 0;
        size_t failedUploads = 0;
        size_t queuedItems = 0;
        double averageUploadTimeMs = 0.0;
    };
    
    const UploadStats& GetStats() const { return stats_; }
    void ResetStats();
    
    bool IsUploading() const { return isUploading_; }
    void CancelAll();
    
private:
    TelemetryUploader();
    ~TelemetryUploader();
    
    void UploadWorker();
    bool ProcessTask(UploadTask& task);
    bool PerformUpload(const std::string& endpoint, const std::vector<uint8_t>& data);
    
    bool enabled_ = false;
    std::string endpoint_;
    size_t maxQueueSize_ = 100;
    int maxConcurrentUploads_ = 2;
    std::chrono::seconds uploadInterval_ = std::chrono::seconds(60);
    
    std::queue<UploadTask> uploadQueue_;
    std::vector<std::thread> workerThreads_;
    std::atomic<bool> stopWorkers_{false};
    std::atomic<bool> isUploading_{false};
    
    std::function<void(bool, const std::string&)> uploadCallback_;
    UploadStats stats_;
    
    std::mutex queueMutex_;
    std::condition_variable queueCV_;
};

class MetricsCollector {
public:
    MetricsCollector();
    ~MetricsCollector();
    
    void Start();
    void Stop();
    bool IsRunning() const { return isRunning_; }
    
    void SetCollectionInterval(std::chrono::seconds interval);
    std::chrono::seconds GetCollectionInterval() const { return collectionInterval_; }
    
    void SetBufferSize(size_t maxSamples);
    size_t GetBufferSize() const { return bufferSize_; }
    
    void CollectFrameMetrics();
    void CollectMemoryMetrics();
    void CollectPerformanceMetrics();
    
    std::vector<uint8_t> ExportMetrics();
    
private:
    void CollectionLoop();
    
    bool isRunning_ = false;
    std::chrono::seconds collectionInterval_ = std::chrono::seconds(60);
    size_t bufferSize_ = 3600;
    
    std::thread collectionThread_;
    std::atomic<bool> stopCollection_{false};
    
    struct FrameMetrics {
        int64_t frameNumber = 0;
        double frameTimeMs = 0.0;
        int drawCalls = 0;
        int triangles = 0;
    };
    
    std::vector<FrameMetrics> frameBuffer_;
    size_t currentFrameIndex_ = 0;
};

} // namespace telemetry
} // namespace ge