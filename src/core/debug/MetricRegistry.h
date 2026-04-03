#pragma once

// ================================================================
//  MetricRegistry.h
//  Central registry for all telemetry metrics.
// ================================================================

#include "MetricTypes.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <variant>

namespace ge {
namespace debug {

class MetricRegistry {
public:
    static MetricRegistry& Get();
    
    MetricRegistry() = default;
    ~MetricRegistry() = default;
    
    void Initialize();
    void Shutdown();
    void Reset();
    
    Counter* RegisterCounter(const std::string& name, const std::string& description = "", 
                            const std::string& category = "Default");
    Timer* RegisterTimer(const std::string& name, const std::string& description = "",
                        const std::string& category = "Default");
    Gauge* RegisterGauge(const std::string& name, const std::string& description = "",
                        const std::string& category = "Default");
    Marker* RegisterMarker(const std::string& name, const std::string& description = "",
                           const std::string& category = "Default");
    
    Counter* GetCounter(const std::string& name);
    Timer* GetTimer(const std::string& name);
    Gauge* GetGauge(const std::string& name);
    Marker* GetMarker(const std::string& name);
    
    bool HasMetric(const std::string& name) const;
    void UnregisterMetric(const std::string& name);
    
    std::vector<std::string> GetMetricNames(MetricType type = MetricType::Counter) const;
    std::vector<std::string> GetCategories() const;
    std::vector<std::string> GetMetricsInCategory(const std::string& category) const;
    
    void SetUpdateCallback(std::function<void(const std::string& name, const MetricValue&)> callback);
    
    struct AggregatedMetrics {
        int64_t frameCount = 0;
        double fps = 0.0;
        double avgFrameTime = 0.0;
        double minFrameTime = 0.0;
        double maxFrameTime = 0.0;
        int64_t totalDrawCalls = 0;
        int64_t totalTriangles = 0;
        int64_t memoryUsed = 0;
        int64_t memoryPeak = 0;
        double cpuUsage = 0.0;
        double gpuUsage = 0.0;
        std::unordered_map<std::string, double> customCounters;
        std::unordered_map<std::string, double> customGauges;
    };
    
    AggregatedMetrics ComputeAggregates();
    
    void SetCollectionEnabled(bool enabled);
    bool IsCollectionEnabled() const { return collectionEnabled_; }
    
    void SetSampleRate(int samplesPerSecond);
    int GetSampleRate() const { return sampleRate_; }
    
private:
    struct MetricEntry {
        std::string name;
        std::string description;
        std::string category;
        MetricType type;
        enum class Kind { None, Counter, Timer, Gauge, Marker } kind = Kind::None;
        union {
            Counter* counter = nullptr;
            Timer* timer;
            Gauge* gauge;
            Marker* marker;
        };
    };
    
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, MetricEntry> metrics_;
    std::unordered_map<std::string, std::vector<std::string>> categoryMap_;
    
    std::function<void(const std::string&, const MetricValue&)> updateCallback_;
    
    bool collectionEnabled_ = true;
    int sampleRate_ = 60;
};

class ScopedMetric {
public:
    ScopedMetric(const std::string& name);
    ~ScopedMetric();
    
    void SetLabel(const std::string& label);
    void SetValue(const MetricValue& value);
    
private:
    std::string name_;
    std::string label_;
    MetricValue value_;
};

} // namespace debug
} // namespace ge
