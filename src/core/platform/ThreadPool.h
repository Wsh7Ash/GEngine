#pragma once

// ================================================================
//  ThreadPool.h
//  Cross-platform thread pool abstraction.
// ================================================================

#include "Thread.h"
#include <functional>
#include <memory>
#include <vector>
#include <queue>
#include <atomic>
#include <future>
#include <type_traits>

namespace ge {
namespace platform {

class ThreadPool {
public:
    static std::unique_ptr<ThreadPool> Create(int32_t threadCount = -1);
    static ThreadPool& GetGlobal();

    virtual ~ThreadPool() = default;

    virtual void Initialize(int32_t threadCount = -1) = 0;
    virtual void Shutdown() = 0;

    virtual void Submit(std::function<void()> task) = 0;
    virtual void SubmitPriority(std::function<void()> task, int32_t priority = 0) = 0;

    virtual size_t GetQueueSize() const = 0;
    virtual size_t GetThreadCount() const = 0;
    virtual size_t GetActiveThreadCount() const = 0;

    virtual void SetThreadCount(int32_t count) = 0;
    virtual void SetThreadPriority(int32_t threadIndex, int32_t priority) = 0;

    virtual void Pause() = 0;
    virtual void Resume() = 0;
    virtual bool IsPaused() const = 0;

    virtual void WaitForIdle() = 0;

    virtual void Trim() = 0;

    template<typename Func, typename... Args>
    auto Enqueue(Func&& func, Args&&... args) -> std::future<typename std::result_of<Func(Args...)>::type> {
        using ReturnType = typename std::result_of<Func(Args...)>::type;
        
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            [func, args...]()
            {
                return func(args...);
            }
        );
        auto result = task->get_future();
        
        Submit([task]() { (*task)(); });
        return result;
    }

protected:
    ThreadPool() = default;
};

class TaskQueue {
public:
    using Task = std::function<void()>;
    using Priority = int32_t;

    virtual ~TaskQueue() = default;

    virtual void Push(Task task, Priority priority = 0) = 0;
    virtual bool Pop(Task& task) = 0;
    virtual bool TryPop(Task& task) = 0;

    virtual size_t Size() const = 0;
    virtual bool Empty() const = 0;
    virtual void Clear() = 0;

    virtual void SetCapacity(size_t maxSize) = 0;
    virtual size_t GetCapacity() const = 0;
};

class WorkStealingQueue {
public:
    virtual ~WorkStealingQueue() = default;

    virtual void Push(void* task) = 0;
    virtual void* Pop() = 0;
    virtual void* Steal() = 0;

    virtual bool Empty() const = 0;
    virtual size_t Size() const = 0;
};

class ThreadPoolStats {
public:
    ThreadPoolStats() = default;
    ~ThreadPoolStats() = default;

    size_t totalTasksSubmitted = 0;
    size_t totalTasksCompleted = 0;
    size_t totalTasksCancelled = 0;
    size_t peakQueueSize = 0;
    double avgQueueWaitTimeMs = 0.0;
    double avgTaskExecutionTimeMs = 0.0;
    double totalCpuTimeMs = 0.0;

    struct ThreadStats {
        uint64_t tasksCompleted = 0;
        double totalTimeMs = 0.0;
        double idleTimeMs = 0.0;
        double stealCount = 0.0;
    };

    std::vector<ThreadStats> threadStats;
};

class IThreadWorker {
public:
    virtual ~IThreadWorker() = default;
    virtual void Initialize(ThreadPool* pool, int32_t index) = 0;
    virtual void Execute() = 0;
    virtual void Shutdown() = 0;
    virtual void NotifyWorkAvailable() = 0;
};

} // namespace platform
} // namespace ge
