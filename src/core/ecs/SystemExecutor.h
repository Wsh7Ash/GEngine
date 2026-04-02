#pragma once

// ================================================================
//  SystemExecutor.h
//  Parallel system executor using thread pool.
// ================================================================

#include "System.h"
#include "SystemGraph.h"
#include "World.h"
#include <vector>
#include <thread>
#include <atomic>
#include <functional>
#include <condition_variable>
#include <queue>

namespace ge {
namespace ecs {

class SystemExecutor {
public:
    SystemExecutor();
    explicit SystemExecutor(int threadCount);
    ~SystemExecutor();
    
    void Initialize(int threadCount = -1);
    void Shutdown();
    
    void BuildExecutionGraph(SystemManager* systemManager);
    
    void Update(World& world, float dt);
    
    void SetEnabled(bool enabled) { isEnabled_ = enabled; }
    bool IsEnabled() const { return isEnabled_; }
    
    void SetParallelThreshold(float ms) { parallelThreshold_ = ms; }
    float GetParallelThreshold() const { return parallelThreshold_; }
    
    const SystemGraph& GetGraph() const { return systemGraph_; }
    SystemGraph& GetGraph() { return systemGraph_; }
    
    int GetThreadCount() const { return threadCount_; }
    
    float GetLastFrameTime() const { return lastFrameTime_; }
    float GetTotalSystemTime() const { return totalSystemTime_; }

private:
    void WorkerThread();
    void ExecuteFrame(int frameIndex, World& world, float dt);
    void ExecuteSystem(System* system, World& world, float dt);
    
    void SubmitTask(std::function<void()> task);
    void WaitForTasks();
    
    std::vector<std::thread> workerThreads_;
    std::queue<std::function<void()>> taskQueue_;
    std::atomic<int> activeTasks_{0};
    std::atomic<bool> shutdown_{false};
    std::mutex queueMutex_;
    std::condition_variable queueCV_;
    
    SystemGraph systemGraph_;
    int threadCount_ = 0;
    bool isEnabled_ = true;
    bool isInitialized_ = false;
    
    float parallelThreshold_ = 2.0f;
    float lastFrameTime_ = 0.0f;
    float totalSystemTime_ = 0.0f;
    
    std::atomic<uint64_t> frameNumber_{0};
};

} // namespace ecs
} // namespace ge
