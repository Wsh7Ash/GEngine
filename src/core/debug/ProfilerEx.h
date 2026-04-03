#pragma once

// ================================================================
//  ProfilerEx.h
//  Extended profiler with thread-awareness and GPU timing.
// ================================================================

#include "Profiler.h"
#include "MetricTypes.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <thread>
#include <atomic>
#include <mutex>
#include <future>

namespace ge {
namespace debug {

struct ProfileThreadData {
    uint32_t threadId = 0;
    std::string threadName;
    std::vector<ProfileNode> nodes;
    std::vector<int> nodeStack;
    int64_t frameStartTime = 0;
    
    std::atomic<bool> isProfiling{false};
};

struct GPUMarker {
    std::string name;
    int64_t startTime = 0;
    int64_t endTime = 0;
    uint32_t queryId = 0;
};

struct ContinuousSample {
    int64_t timestamp = 0;
    std::string name;
    int64_t startTime = 0;
    int64_t endTime = 0;
    uint32_t threadId = 0;
    int depth = 0;
    float percentage = 0.0f;
};

class ProfilerEx {
public:
    static ProfilerEx& Get();
    
    ProfilerEx();
    ~ProfilerEx();
    
    void Initialize();
    void Shutdown();
    
    void BeginFrame();
    void EndFrame();
    
    void BeginScope(const char* name, uint32_t threadId = 0);
    void EndScope(const char* name, uint32_t threadId = 0);
    
    void BeginScopeThread(uint32_t threadId, const char* threadName);
    void EndScopeThread(uint32_t threadId);
    
    void SetThreadName(uint32_t threadId, const std::string& name);
    std::string GetThreadName(uint32_t threadId) const;
    
    void AddGPUMarker(const std::string& name, int64_t start, int64_t end);
    
    const std::vector<ContinuousSample>& GetContinuousSamples() const { return continuousSamples_; }
    const std::unordered_map<uint32_t, ProfileThreadData>& GetThreadData() const { return threadData_; }
    
    void SetMaxContinuousSamples(size_t max);
    size_t GetMaxContinuousSamples() const { return maxContinuousSamples_; }
    
    void SetCaptureEnabled(bool enabled);
    bool IsCaptureEnabled() const { return captureEnabled_; }
    
    void StartContinuousCapture();
    void StopContinuousCapture();
    bool IsContinuousCapturing() const { return continuousCapture_; }
    
    void SetGPUCalibrationFactor(double microsecondsPerTick);
    double GetGPUCalibrationFactor() const { return gpuCalibration_; }
    
    void SetProfileThisFrame(const std::string& passName);
    bool ShouldProfileThisFrame(const std::string& passName) const;
    
    void SetFrameBudget(double microseconds);
    double GetFrameBudget() const { return frameBudget_; }
    
    bool IsFrameOverBudget() const { return isOverBudget_.load(); }
    double GetFrameOverBudgetPercent() const { return overBudgetPercent_; }
    
    struct FrameStatistics {
        int64_t frameTimeUs = 0;
        double frameTimeMs = 0.0;
        double fps = 0.0;
        int scopeCount = 0;
        int peakDepth = 0;
        size_t memoryUsed = 0;
        size_t memoryPeak = 0;
        std::vector<std::string> slowPasses;
    };
    
    const FrameStatistics& GetFrameStats() const { return frameStats_; }
    FrameStatistics& GetFrameStats() { return frameStats_; }
    
    void EnableThreadValidation(bool enable);
    bool IsThreadValidationEnabled() const { return validateThread_; }
    
    void SetMaxDepth(int max);
    int GetMaxDepth() const { return maxDepth_; }
    
    void SetWarningThreshold(double thresholdMs);
    double GetWarningThreshold() const { return warningThreshold_; }
    
    std::vector<std::string> GetWarnings() const;
    void ClearWarnings();
    
    std::string ExportToChromeTracing() const;
    std::string ExportToJSON() const;
    
private:
    ProfileThreadData& GetOrCreateThreadData(uint32_t threadId);
    ProfileThreadData* GetThreadDataPtr(uint32_t threadId);
    void ProcessFrameData();
    void CalculateContinuousSamples();
    
    std::unordered_map<uint32_t, ProfileThreadData> threadData_;
    std::unordered_map<std::string, uint32_t> threadNameToId_;
    std::vector<GPUMarker> gpuMarkers_;
    
    std::vector<ContinuousSample> continuousSamples_;
    std::vector<ContinuousSample> pendingSamples_;
    std::vector<std::string> warnings_;
    
    std::atomic<uint32_t> currentThreadId_;
    std::atomic<bool> captureEnabled_{true};
    std::atomic<bool> continuousCapture_{false};
    std::atomic<bool> isOverBudget_{false};
    
    size_t maxContinuousSamples_ = 10000;
    double gpuCalibration_ = 1.0;
    double frameBudget_ = 16666.0;
    double overBudgetPercent_ = 0.0;
    int maxDepth_ = 32;
    double warningThreshold_ = 10.0;
    bool validateThread_ = false;
    
    std::mutex threadDataMutex_;
    FrameStatistics frameStats_;
    
    std::string profilePassFilter_;
};

class ScopedThreadProfile {
public:
    ScopedThreadProfile(const char* threadName);
    ~ScopedThreadProfile();
    
private:
    uint32_t threadId_;
};

} // namespace debug
} // namespace ge
