// ================================================================
//  ThreadSafetyValidator.h
//  Thread safety validation for ECS stress testing.
// ================================================================

#pragma once

#include "../core/ecs/World.h"
#include <atomic>
#include <thread>
#include <vector>
#include <functional>
#include <string>
#include <unordered_map>
#include <mutex>

namespace ge {
namespace tests {

enum class ValidationSeverity {
    Info,
    Warning,
    Error,
    Critical
};

struct ValidationIssue {
    ValidationSeverity severity;
    std::string message;
    std::string location;
    std::string threadContext;
    int64_t timestamp;
};

class ThreadSafetyValidator {
public:
    ThreadSafetyValidator();
    ~ThreadSafetyValidator();

    void SetWorld(ecs::World* world);
    ecs::World* GetWorld() const { return world_; }

    bool Validate();

    void SetValidationDuration(double seconds);
    double GetValidationDuration() const { return validationDuration_; }

    void SetThreadCount(int count);
    int GetThreadCount() const { return threadCount_; }

    void SetParallelWorkerThreads(int count);
    int GetParallelWorkerThreads() const { return parallelWorkers_; }

    const std::vector<ValidationIssue>& GetIssues() const { return issues_; }
    bool HasErrors() const;
    bool HasWarnings() const;

    void SetValidationCallback(std::function<void(const ValidationIssue&)> callback);

    struct ComponentAccessStats {
        std::atomic<uint64_t> readCount{0};
        std::atomic<uint64_t> writeCount{0};
        std::atomic<uint64_t> conflictCount{0};
    };

    ComponentAccessStats* GetAccessStats(ecs::ComponentTypeID componentId);
    void ResetStats();

    void SetLockTimeoutMs(int ms);
    int GetLockTimeoutMs() const { return lockTimeoutMs_; }

    bool IsThreadSafe() const { return threadSafe_; }
    std::string GetValidationSummary() const;

private:
    void ValidateComponentArrays();
    void ValidateEntityManager();
    void ValidateSystemExecutor();
    void ValidateComponentAccess();
    void RunParallelStressTest();

    void LogIssue(ValidationSeverity severity, const std::string& message, const std::string& location = "");

    ecs::World* world_ = nullptr;
    std::vector<ValidationIssue> issues_;

    double validationDuration_ = 5.0;
    int threadCount_ = 4;
    int parallelWorkers_ = 4;
    int lockTimeoutMs_ = 100;
    bool threadSafe_ = true;

    std::unordered_map<ecs::ComponentTypeID, ComponentAccessStats> componentStats_;
    std::mutex logMutex_;

    std::function<void(const ValidationIssue&)> validationCallback_;

    std::atomic<bool> running_{false};
    std::atomic<bool> errorsFound_{false};
};

class LockValidator {
public:
    LockValidator();
    ~LockValidator();

    void RegisterLock(const std::string& name, std::atomic<int>* counter);
    void UnregisterLock(const std::string& name);

    void OnLockAcquired(const std::string& name);
    void OnLockReleased(const std::string& name);

    struct LockStats {
        std::string name;
        uint64_t acquireCount = 0;
        uint64_t releaseCount = 0;
        uint64_t contentionCount = 0;
        double avgWaitTimeMs = 0.0;
        double maxWaitTimeMs = 0.0;
    };

    const std::vector<LockStats>& GetStats() const { return stats_; }
    void Reset();

private:
    struct LockData {
        std::atomic<int>* counter;
        uint64_t acquireCount = 0;
        uint64_t releaseCount = 0;
        uint64_t contentionCount = 0;
        double totalWaitTimeMs = 0.0;
        double maxWaitTimeMs = 0.0;
    };

    std::unordered_map<std::string, LockData> locks_;
    std::vector<LockStats> stats_;
    std::mutex mutex_;
};

class RaceConditionDetector {
public:
    RaceConditionDetector();
    ~RaceConditionDetector();

    void BeginDetection();
    void EndDetection();

    void ReportRead(void* address, int threadId);
    void ReportWrite(void* address, int threadId);
    void ReportReadWrite(void* address, int threadId);

    struct RaceReport {
        void* address;
        std::string variableName;
        std::vector<int> readingThreads;
        std::vector<int> writingThreads;
        int conflictCount;
    };

    const std::vector<RaceReport>& GetReports() const { return reports_; }
    bool HasRaces() const { return !reports_.empty(); }

    void SetDetectionEnabled(bool enabled);
    bool IsDetectionEnabled() const { return detectionEnabled_; }

private:
    struct AccessInfo {
        std::vector<int> readers;
        std::vector<int> writers;
        bool concurrent = false;
    };

    std::unordered_map<void*, AccessInfo> memoryAccess_;
    std::vector<RaceReport> reports_;
    bool detectionEnabled_ = true;
    bool inDetection_ = false;
    std::mutex mutex_;
};

} // namespace tests
} // namespace ge
