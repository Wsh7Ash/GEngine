#pragma once

// ================================================================
//  MetricTypes.h
//  Core metric type definitions for telemetry system.
// ================================================================

#include <string>
#include <vector>
#include <atomic>
#include <chrono>
#include <cstdint>

namespace ge {
namespace debug {

enum class MetricType {
    Counter,
    Timer,
    Gauge,
    Marker
};

enum class MetricSource {
    CPU,
    GPU,
    Memory,
    Network,
    Custom
};

struct MetricValue {
    enum Tag { Empty, Int, UInt, Float, Double, Bool, String };
    Tag tag = Empty;
    int64_t intVal = 0;
    uint64_t uintVal = 0;
    float floatVal = 0.0f;
    double doubleVal = 0.0f;
    bool boolVal = false;
    std::string stringVal;
    
    static MetricValue FromInt(int64_t v) { MetricValue m; m.tag = Int; m.intVal = v; return m; }
    static MetricValue FromUInt(uint64_t v) { MetricValue m; m.tag = UInt; m.uintVal = v; return m; }
    static MetricValue FromFloat(float v) { MetricValue m; m.tag = Float; m.floatVal = v; return m; }
    static MetricValue FromDouble(double v) { MetricValue m; m.tag = Double; m.doubleVal = v; return m; }
    static MetricValue FromBool(bool v) { MetricValue m; m.tag = Bool; m.boolVal = v; return m; }
    static MetricValue FromString(const std::string& v) { MetricValue m; m.tag = String; m.stringVal = v; return m; }
};

struct MetricDefinition {
    std::string name;
    std::string description;
    std::string category;
    MetricType type;
    MetricSource source;
    MetricValue minValue;
    MetricValue maxValue;
    MetricValue defaultValue;
    
    bool operator<(const MetricDefinition& other) const { return name < other.name; }
};

struct MetricSample {
    std::string metricName;
    MetricValue value;
    int64_t timestamp_us;
    uint32_t threadId;
};

class Counter {
public:
    Counter() = default;
    explicit Counter(const std::string& name);
    
    void Increment(int64_t delta = 1);
    void Decrement(int64_t delta = 1);
    void Set(int64_t value);
    int64_t Get() const { return value_.load(std::memory_order_relaxed); }
    void Reset();
    
    const std::string& GetName() const { return name_; }
    
private:
    std::string name_;
    std::atomic<int64_t> value_{0};
};

class Timer {
public:
    Timer() = default;
    explicit Timer(const std::string& name);
    
    void Start();
    void Stop();
    void Reset();
    
    bool IsRunning() const { return running_.load(std::memory_order_relaxed); }
    int64_t GetElapsedMicroseconds() const;
    double GetElapsedMilliseconds() const;
    double GetElapsedSeconds() const;
    
    const std::string& GetName() const { return name_; }
    
private:
    std::string name_;
    std::atomic<bool> running_{false};
    std::atomic<int64_t> accumulated_{0};
    int64_t startTime_ = 0;
};

class Gauge {
public:
    Gauge() = default;
    explicit Gauge(const std::string& name);
    
    void Set(double value);
    void Set(int64_t value);
    void Increment(double delta = 1.0);
    void Decrement(double delta = 1.0);
    double Get() const { return value_.load(std::memory_order_relaxed); }
    void Reset();
    
    const std::string& GetName() const { return name_; }
    
private:
    std::string name_;
    std::atomic<double> value_{0.0};
};

class Marker {
public:
    Marker() = default;
    explicit Marker(const std::string& name);
    
    void Mark(const std::string& label = "");
    void Mark(const std::string& label, const MetricValue& value);
    
    struct Event {
        std::string label;
        MetricValue value;
        int64_t timestamp_us;
    };
    
    const std::vector<Event>& GetEvents() const;
    void Clear();
    
    const std::string& GetName() const { return name_; }
    
private:
    std::string name_;
    std::vector<Event> events_;
};

class ScopedTimer {
public:
    ScopedTimer(Timer& timer);
    ScopedTimer(Timer* timer);
    ~ScopedTimer();
    
    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;
    
    ScopedTimer(ScopedTimer&& other) noexcept;
    ScopedTimer& operator=(ScopedTimer&& other) noexcept;
    
private:
    Timer* timer_ = nullptr;
};

} // namespace debug
} // namespace ge

#define GE_METRIC_COUNTER(name, desc) static ge::debug::Counter _ge_metric_##name(#name)
#define GE_METRIC_GAUGE(name, desc) static ge::debug::Gauge _ge_metric_##name(#name)
#define GE_METRIC_TIMER(name, desc) static ge::debug::Timer _ge_metric_##name(#name)
#define GE_METRIC_MARKER(name, desc) static ge::debug::Marker _ge_metric_##name(#name)

#if defined(GE_DEBUG) || defined(GE_ENABLE_PROFILING)
    #define GE_METRIC_SCOPE(name) ge::debug::Timer _ge_timer_##name(#name); _ge_timer_##name.Start()
#else
    #define GE_METRIC_SCOPE(name)
#endif
