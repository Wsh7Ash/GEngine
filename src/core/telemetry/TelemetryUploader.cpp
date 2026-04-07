#include "TelemetryUploader.h"
#include "../debug/log.h"
#include "../debug/Profiler.h"

#include <chrono>
#include <fstream>

namespace ge {
namespace telemetry {

TelemetryUploader& TelemetryUploader::Get() {
    static TelemetryUploader instance;
    return instance;
}

TelemetryUploader::TelemetryUploader() {
    endpoint_ = "https://telemetry.gengine.dev/v1/";
}

TelemetryUploader::~TelemetryUploader() {
    Shutdown();
}

void TelemetryUploader::Initialize() {
    if (enabled_) return;
    
    enabled_ = true;
    
    for (int i = 0; i < maxConcurrentUploads_; ++i) {
        workerThreads_.emplace_back(&TelemetryUploader::UploadWorker, this);
    }
    
    GE_LOG_INFO("TelemetryUploader initialized with endpoint: {}", endpoint_);
}

void TelemetryUploader::Shutdown() {
    if (!enabled_) return;
    
    enabled_ = false;
    stopWorkers_ = true;
    queueCV_.notify_all();
    
    for (auto& thread : workerThreads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    workerThreads_.clear();
}

void TelemetryUploader::SetEndpoint(const std::string& endpoint) {
    endpoint_ = endpoint;
}

void TelemetryUploader::SetEnabled(bool enabled) {
    if (enabled && !enabled_) {
        Initialize();
    } else if (!enabled && enabled_) {
        Shutdown();
    }
}

void TelemetryUploader::SetMaxQueueSize(size_t maxSize) {
    maxQueueSize_ = maxSize;
}

size_t TelemetryUploader::GetQueueSize() const {
    return uploadQueue_.size();
}

void TelemetryUploader::SetMaxConcurrentUploads(int max) {
    maxConcurrentUploads_ = max;
}

void TelemetryUploader::SetUploadInterval(std::chrono::seconds interval) {
    uploadInterval_ = interval;
}

bool TelemetryUploader::QueueSessionUpload(debug::TelemetrySession* session) {
    if (!enabled_) return false;
    
    UploadTask task;
    task.type = UploadTask::Type::TelemetrySession;
    task.endpoint = endpoint_ + "telemetry";
    task.data = std::vector<uint8_t>(session->ExportToString(debug::ExportFormat::Binary).begin(),
                                      session->ExportToString(debug::ExportFormat::Binary).end());
    task.priority = 1;
    
    std::lock_guard<std::mutex> lock(queueMutex_);
    if (uploadQueue_.size() < maxQueueSize_) {
        uploadQueue_.push(std::move(task));
        queueCV_.notify_one();
        return true;
    }
    
    return false;
}

bool TelemetryUploader::QueueCrashUpload(const std::string& crashPath) {
    if (!enabled_) return false;
    
    std::ifstream file(crashPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;
    
    size_t size = file.tellg();
    file.seekg(0);
    
    std::vector<uint8_t> data(size);
    file.read(reinterpret_cast<char*>(data.data()), size);
    
    UploadTask task;
    task.type = UploadTask::Type::CrashReport;
    task.endpoint = endpoint_ + "crash";
    task.data = std::move(data);
    task.priority = 0;
    
    std::lock_guard<std::mutex> lock(queueMutex_);
    if (uploadQueue_.size() < maxQueueSize_) {
        uploadQueue_.push(std::move(task));
        queueCV_.notify_one();
        return true;
    }
    
    return false;
}

bool TelemetryUploader::QueueMetricsUpload(const std::vector<uint8_t>& metricsData) {
    if (!enabled_) return false;
    
    UploadTask task;
    task.type = UploadTask::Type::MetricsBatch;
    task.endpoint = endpoint_ + "metrics";
    task.data = metricsData;
    task.priority = 2;
    
    std::lock_guard<std::mutex> lock(queueMutex_);
    if (uploadQueue_.size() < maxQueueSize_) {
        uploadQueue_.push(std::move(task));
        queueCV_.notify_one();
        return true;
    }
    
    return false;
}

void TelemetryUploader::SetUploadCallback(std::function<void(bool, const std::string&)> callback) {
    uploadCallback_ = callback;
}

void TelemetryUploader::ResetStats() {
    stats_ = UploadStats();
}

void TelemetryUploader::CancelAll() {
    std::lock_guard<std::mutex> lock(queueMutex_);
    while (!uploadQueue_.empty()) {
        uploadQueue_.pop();
    }
}

void TelemetryUploader::UploadWorker() {
    while (!stopWorkers_) {
        UploadTask task;
        
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            queueCV_.wait_for(lock, uploadInterval_, [this] { 
                return !uploadQueue_.empty() || stopWorkers_; 
            });
            
            if (stopWorkers_ || uploadQueue_.empty()) continue;
            task = std::move(uploadQueue_.front());
            uploadQueue_.pop();
        }
        
        bool success = ProcessTask(task);
        
        if (uploadCallback_) {
            uploadCallback_(success, task.endpoint);
        }
    }
}

bool TelemetryUploader::ProcessTask(UploadTask& task) {
    isUploading_ = true;
    
    auto startTime = std::chrono::steady_clock::now();
    bool success = PerformUpload(task.endpoint, task.data);
    auto endTime = std::chrono::steady_clock::now();
    
    double uploadTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    
    stats_.totalUploads++;
    if (success) {
        stats_.successfulUploads++;
    } else {
        stats_.failedUploads++;
        
        if (task.retryCount < task.maxRetries) {
            task.retryCount++;
            std::lock_guard<std::mutex> lock(queueMutex_);
            uploadQueue_.push(std::move(task));
        }
    }
    
    stats_.averageUploadTimeMs = (stats_.averageUploadTimeMs * (stats_.totalUploads - 1) + uploadTimeMs) / stats_.totalUploads;
    stats_.queuedItems = uploadQueue_.size();
    
    isUploading_ = false;
    return success;
}

bool TelemetryUploader::PerformUpload(const std::string& endpoint, const std::vector<uint8_t>& data) {
    GE_LOG_INFO("Uploading {} bytes to {}", data.size(), endpoint);
    return true;
}

MetricsCollector::MetricsCollector() {
    frameBuffer_.reserve(bufferSize_);
}

MetricsCollector::~MetricsCollector() {
    Stop();
}

void MetricsCollector::Start() {
    if (isRunning_) return;
    
    isRunning_ = true;
    stopCollection_ = false;
    collectionThread_ = std::thread(&MetricsCollector::CollectionLoop, this);
}

void MetricsCollector::Stop() {
    if (!isRunning_) return;
    
    isRunning_ = false;
    stopCollection_ = true;
    
    if (collectionThread_.joinable()) {
        collectionThread_.join();
    }
}

void MetricsCollector::SetCollectionInterval(std::chrono::seconds interval) {
    collectionInterval_ = interval;
}

void MetricsCollector::SetBufferSize(size_t maxSamples) {
    bufferSize_ = maxSamples;
    frameBuffer_.reserve(bufferSize_);
}

void MetricsCollector::CollectFrameMetrics() {
    FrameMetrics metrics;
    metrics.frameNumber = debug::Profiler::Get().GetFrameTime();
    metrics.frameTimeMs = debug::Profiler::Get().GetFrameTime();
    metrics.drawCalls = 0;
    metrics.triangles = 0;
    
    if (frameBuffer_.size() < bufferSize_) {
        frameBuffer_.push_back(metrics);
    } else {
        frameBuffer_[currentFrameIndex_] = metrics;
        currentFrameIndex_ = (currentFrameIndex_ + 1) % bufferSize_;
    }
}

void MetricsCollector::CollectMemoryMetrics() {
}

void MetricsCollector::CollectPerformanceMetrics() {
}

std::vector<uint8_t> MetricsCollector::ExportMetrics() {
    return std::vector<uint8_t>();
}

void MetricsCollector::CollectionLoop() {
    while (!stopCollection_) {
        std::this_thread::sleep_for(collectionInterval_);
        
        if (!stopCollection_) {
            CollectFrameMetrics();
            CollectMemoryMetrics();
            CollectPerformanceMetrics();
        }
    }
}

} // namespace telemetry
} // namespace ge