#pragma once

// ================================================================
//  RenderQueue.h
//  Priority queue with sort keys for command buffer merging.
// ================================================================

#include "CommandBuffer.h"
#include <vector>
#include <algorithm>
#include <atomic>

namespace ge {
namespace renderer {

enum class SortOrder {
    FrontToBack,
    BackToFront,
    MaterialFirst,
    None
};

struct RenderQueueConfig {
    SortOrder opaqueSort = SortOrder::FrontToBack;
    SortOrder transparentSort = SortOrder::BackToFront;
    bool enableSorting = true;
    bool enableInstancing = true;
    size_t maxCommandsPerPass = 65536;
};

class RenderQueue {
public:
    RenderQueue();
    explicit RenderQueue(const RenderQueueConfig& config);
    ~RenderQueue();
    
    void Initialize();
    void Shutdown();
    
    void BeginFrame();
    void EndFrame();
    
    void AddCommand(const DrawCommand& command);
    void AddCommands(const DrawCommand* commands, size_t count);
    
    void MergeBuffer(CommandBuffer* buffer);
    
    void Sort();
    void SortOpaque();
    void SortTransparent();
    
    std::vector<DrawCommand>& GetOpaqueCommands() { return opaqueCommands_; }
    std::vector<DrawCommand>& GetTransparentCommands() { return transparentCommands_; }
    std::vector<DrawCommand>& GetUICommands() { return uiCommands_; }
    
    const std::vector<DrawCommand>& GetOpaqueCommands() const { return opaqueCommands_; }
    const std::vector<DrawCommand>& GetTransparentCommands() const { return transparentCommands_; }
    const std::vector<DrawCommand>& GetUICommands() const { return uiCommands_; }
    
    size_t GetTotalCommandCount() const;
    size_t GetOpaqueCount() const { return opaqueCommands_.size(); }
    size_t GetTransparentCount() const { return transparentCommands_.size(); }
    size_t GetUICount() const { return uiCommands_.size(); }
    
    void Clear();
    
    void SetConfig(const RenderQueueConfig& config);
    const RenderQueueConfig& GetConfig() const { return config_; }

private:
    void MergeSortedVectors(std::vector<DrawCommand>& dest, const std::vector<DrawCommand>& source);
    uint64_t GetSortKeyForCommand(const DrawCommand& cmd) const;
    
    RenderQueueConfig config_;
    
    std::vector<DrawCommand> opaqueCommands_;
    std::vector<DrawCommand> transparentCommands_;
    std::vector<DrawCommand> uiCommands_;
    std::vector<DrawCommand> shadowCommands_;
    
    std::atomic<uint32_t> totalCommandCount_{0};
    uint32_t currentFrame_ = 0;
    
    bool isSorted_ = false;
};

class RenderQueueManager {
public:
    static RenderQueueManager& Get();
    
    RenderQueueManager();
    ~RenderQueueManager();
    
    void Initialize(size_t workerThreadCount);
    void Shutdown();
    
    void BeginFrame();
    void EndFrame();
    void Present();
    
    RenderQueue* GetMainQueue() { return &mainQueue_; }
    RenderQueue& GetQueue() { return mainQueue_; }
    
    void SubmitThreadBuffer(CommandBuffer* buffer);
    
    size_t GetWorkerThreadCount() const { return workerThreadCount_; }
    
    void SetEnabled(bool enabled) { enabled_ = enabled; }
    bool IsEnabled() const { return enabled_; }

private:
    void ProcessThreadBuffers();
    
    RenderQueue mainQueue_;
    std::vector<CommandBuffer*> threadBuffers_;
    std::vector<CommandBuffer*> pendingBuffers_;
    
    size_t workerThreadCount_ = 0;
    bool enabled_ = true;
    bool isInitialized_ = false;
    
    std::mutex queueMutex_;
    std::atomic<bool> frameComplete_{false};
    
    uint32_t frameNumber_ = 0;
};

} // namespace renderer
} // namespace ge
