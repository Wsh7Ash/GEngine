#pragma once

// ================================================================
//  MetricBuffer.h
//  Lock-free ring buffer for streaming metrics.
// ================================================================

#include "MetricTypes.h"
#include "TelemetrySession.h"
#include <atomic>
#include <vector>
#include <cstring>
#include <memory>

namespace ge {
namespace debug {

class MetricBuffer {
public:
    MetricBuffer();
    explicit MetricBuffer(size_t capacity);
    ~MetricBuffer();
    
    bool Write(const MetricSample& sample);
    bool Write(const MetricSample* samples, size_t count);
    
    bool Read(MetricSample& sample);
    size_t Read(MetricSample* samples, size_t maxCount);
    
    size_t GetCount() const;
    size_t GetCapacity() const { return capacity_; }
    bool IsFull() const;
    bool IsEmpty() const;
    
    void Clear();
    void Reset();
    
    void SetCapacity(size_t newCapacity);
    
    bool TryWrite(const MetricSample& sample);
    size_t TryWrite(const MetricSample* samples, size_t count);
    
    struct Statistics {
        size_t totalWritten = 0;
        size_t totalRead = 0;
        size_t writeOverflows = 0;
        size_t readUnderflows = 0;
    };
    
    const Statistics& GetStatistics() const { return stats_; }
    void ResetStatistics();
    
private:
    size_t GetReadIndex() const;
    size_t GetWriteIndex() const;
    
    static constexpr size_t DEFAULT_CAPACITY = 4096;
    
    MetricSample* buffer_ = nullptr;
    size_t capacity_ = 0;
    
    std::atomic<size_t> writeIndex_{0};
    std::atomic<size_t> readIndex_{0};
    
    Statistics stats_;
};

class StreamingBuffer {
public:
    StreamingBuffer();
    explicit StreamingBuffer(size_t maxSize);
    ~StreamingBuffer();
    
    void AddMetric(const std::string& name, int64_t value);
    void AddMetric(const std::string& name, double value);
    void AddMetric(const std::string& name, const std::string& value);
    
    void AddFrame(int64_t frameId, double frameTimeMs);
    void AddMarker(const std::string& name, int64_t timestamp);
    void AddScope(const std::string& name, int64_t start, int64_t end, int depth = 0);
    
    void BeginFrame(int64_t frameId);
    void EndFrame();
    
    const std::vector<uint8_t>& GetData() const { return data_; }
    size_t GetDataSize() const { return data_.size(); }
    
    void Clear();
    void Reserve(size_t size);
    
    bool IsEnabled() const { return enabled_; }
    void SetEnabled(bool enabled) { enabled_ = enabled; }
    
    void SetMaxSize(size_t maxBytes);
    size_t GetMaxSize() const { return maxSize_; }
    
    void Compress();
    bool IsCompressed() const { return compressed_; }
    
private:
    void SerializeHeader();
    void SerializeFrame();
    void SerializeMetric(const std::string& name, const MetricValue& value);
    
    std::vector<uint8_t> data_;
    std::vector<MetricSample> pendingMetrics_;
    
    int64_t currentFrameId_ = 0;
    double currentFrameTime_ = 0.0;
    bool enabled_ = true;
    size_t maxSize_ = 1024 * 1024;
    bool compressed_ = false;
    
    bool inFrame_ = false;
};

class Aggregator {
public:
    Aggregator();
    ~Aggregator();
    
    void SetWindowSize(int frameCount);
    int GetWindowSize() const { return windowSize_; }
    
    void AddSample(double value);
    void AddSample(int64_t value);
    
    double GetAverage() const;
    double GetMin() const;
    double GetMax() const;
    double GetSum() const;
    int GetCount() const;
    
    double GetPercentile(double p) const;
    double GetStdDev() const;
    
    void Reset();
    
    double GetAveragePerSecond(double frameTimeMs) const;
    
private:
    std::vector<double> samples_;
    int windowSize_ = 60;
    int currentIndex_ = 0;
    bool filled_ = false;
};

} // namespace debug
} // namespace ge
