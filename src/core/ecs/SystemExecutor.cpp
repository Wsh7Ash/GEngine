#include "ecs/SystemExecutor.h"
#include "debug/log.h"

namespace ge {
namespace ecs {

SystemExecutor::SystemExecutor() {
    int cores = (int)std::thread::hardware_concurrency();
    threadCount_ = cores > 1 ? cores - 1 : 1;
}

SystemExecutor::SystemExecutor(int threadCount) : threadCount_(threadCount) {}

SystemExecutor::~SystemExecutor() {
    Shutdown();
}

void SystemExecutor::Initialize(int threadCount) {
    if (threadCount > 0) {
        threadCount_ = threadCount;
    } else if (threadCount == -1) {
        int cores = (int)std::thread::hardware_concurrency();
        threadCount_ = cores > 1 ? cores - 1 : 1;
    }
    
    for (int i = 0; i < threadCount_; ++i) {
        workerThreads_.emplace_back(&SystemExecutor::WorkerThread, this);
    }
    
    isInitialized_ = true;
    GE_CORE_INFO("SystemExecutor initialized with {} threads", threadCount_);
}

void SystemExecutor::Shutdown() {
    if (!isInitialized_) return;
    
    shutdown_ = true;
    queueCV_.notify_all();
    
    for (auto& thread : workerThreads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    workerThreads_.clear();
    isInitialized_ = false;
}

void SystemExecutor::BuildExecutionGraph(SystemManager* systemManager) {
    if (!isInitialized_) {
        Initialize();
    }
    systemGraph_.Build(systemManager);
    
    if (systemGraph_.HasConflicts()) {
        GE_CORE_WARN("SystemGraph: Some systems have write conflicts, running sequentially");
    }
}

void SystemExecutor::Update(World& world, float dt) {
    if (!isEnabled_) return;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    const auto& frames = systemGraph_.GetFrames();
    
    for (size_t i = 0; i < frames.size(); ++i) {
        const auto& frame = frames[i];
        
        if (frame.canRunParallel && frame.estimatedTime >= parallelThreshold_) {
            for (auto* node : frame.systems) {
                SubmitTask([this, node, &world, dt]() {
                    ExecuteSystem(node->system, world, dt);
                });
            }
            WaitForTasks();
        } else {
            for (auto* node : frame.systems) {
                ExecuteSystem(node->system, world, dt);
            }
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    lastFrameTime_ = std::chrono::duration<float, std::milli>(endTime - startTime).count();
    totalSystemTime_ += lastFrameTime_;
    
    frameNumber_++;
}

void SystemExecutor::ExecuteSystem(System* system, World& world, float dt) {
    if (system) {
        system->Update(world, dt);
    }
}

void SystemExecutor::WorkerThread() {
    while (!shutdown_) {
        std::function<void()> task;
        
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            queueCV_.wait_for(lock, std::chrono::milliseconds(100),
                [this] { return !taskQueue_.empty() || shutdown_; });
            
            if (shutdown_ && taskQueue_.empty()) {
                return;
            }
            
            if (!taskQueue_.empty()) {
                task = std::move(taskQueue_.front());
                taskQueue_.pop();
            }
        }
        
        if (task) {
            ++activeTasks_;
            task();
            --activeTasks_;
        }
    }
}

void SystemExecutor::SubmitTask(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        taskQueue_.push(std::move(task));
    }
    queueCV_.notify_one();
}

void SystemExecutor::WaitForTasks() {
    while (activeTasks_ > 0) {
        std::this_thread::yield();
    }
}

} // namespace ecs
} // namespace ge
