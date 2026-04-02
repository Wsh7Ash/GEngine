#pragma once

// ================================================================
//  CommandBuffer.h
//  Thread-local command buffers for multi-threaded draw command generation.
// ================================================================

#include "../math/VecTypes.h"
#include "../math/Mat4x4.h"
#include <cstdint>
#include <vector>
#include <atomic>
#include <memory>
#include <thread>
#include <mutex>

namespace ge {
namespace renderer {

enum class RenderPassType : uint8_t {
    Shadow = 0,
    GBuffer = 1,
    Opaque = 2,
    Transparent = 3,
    PostProcess = 4,
    UI = 5,
    Overlay = 6
};

enum class DrawCommandType : uint8_t {
    DrawMesh = 0,
    DrawInstanced = 1,
    DrawFullscreen = 2,
    DrawSkybox = 3,
    DispatchCompute = 4,
    SetState = 5
};

struct DrawCommand {
    uint64_t sortKey;
    uint32_t commandType;
    
    uint32_t meshID;
    uint32_t materialID;
    uint32_t indexCount;
    uint32_t instanceCount;
    
    Math::Mat4f transform;
    Math::Mat4f prevTransform;
    
    float depth;
    uint8_t pass;
    uint8_t flags;
    
    uint32_t padding[2];
    
    DrawCommand()
        : sortKey(0), commandType(0), meshID(0), materialID(0),
          indexCount(0), instanceCount(1), depth(0.0f), pass(0), flags(0) {}
};

struct RenderPass {
    uint32_t type;
    uint32_t framebufferID;
    uint32_t clearFlags;
    Math::Vec4f clearColor;
    float clearDepth;
    uint32_t clearStencil;
    
    std::vector<DrawCommand> commands;
    
    RenderPass() : type(0), framebufferID(0), clearFlags(0),
        clearColor({0, 0, 0, 1}), clearDepth(1.0f), clearStencil(0) {}
};

inline uint64_t MakeSortKey(uint32_t materialID, uint32_t pass, float depth, bool transparent) {
    uint64_t matKey = static_cast<uint64_t>(materialID) << 32;
    uint64_t passKey = static_cast<uint64_t>(pass) << 24;
    uint32_t depthBits = transparent 
        ? static_cast<uint32_t>((1.0f - depth) * 16777215.0f)
        : static_cast<uint32_t>(depth * 16777215.0f);
    return matKey | passKey | depthBits;
}

class CommandBuffer {
public:
    CommandBuffer();
    explicit CommandBuffer(size_t capacity);
    ~CommandBuffer();
    
    void Reset();
    void Reserve(size_t capacity);
    
    DrawCommand* Allocate();
    DrawCommand* Allocate(size_t count);
    
    void Submit(DrawCommand&& cmd);
    void Submit(const DrawCommand& cmd);
    
    size_t GetCommandCount() const { return commandCount_; }
    size_t GetCapacity() const { return capacity_; }
    
    DrawCommand* GetCommands() { return commands_.data(); }
    const DrawCommand* GetCommands() const { return commands_.data(); }
    
    void SetFrame(uint32_t frame) { currentFrame_ = frame; }
    uint32_t GetFrame() const { return currentFrame_; }
    
    bool IsFull() const { return commandCount_ >= capacity_; }

private:
    std::vector<DrawCommand> commands_;
    size_t capacity_;
    size_t commandCount_;
    uint32_t currentFrame_;
};

class ThreadLocalCommandBuffer {
public:
    static ThreadLocalCommandBuffer& Get();
    
    CommandBuffer* GetBuffer();
    CommandBuffer* GetNextBuffer();
    
    void Initialize(size_t capacity = 16384);
    void Shutdown();
    
    void BeginFrame();
    void EndFrame();
    
    uint32_t GetThreadId() const { return threadId_; }

private:
    ThreadLocalCommandBuffer();
    ~ThreadLocalCommandBuffer();
    
    CommandBuffer buffer_;
    uint32_t threadId_;
    
    static thread_local ThreadLocalCommandBuffer* currentBuffer_;
};

class CommandBufferPool {
public:
    CommandBufferPool() = default;
    ~CommandBufferPool();
    
    void Initialize(size_t bufferCount, size_t bufferCapacity);
    void Shutdown();
    
    CommandBuffer* Acquire();
    void Release(CommandBuffer* buffer);
    
    size_t GetActiveCount() const { return activeCount_; }
    size_t GetTotalCount() const { return buffers_.size(); }

private:
    std::vector<std::unique_ptr<CommandBuffer>> buffers_;
    std::vector<CommandBuffer*> freeList_;
    std::atomic<size_t> activeCount_{0};
    
    std::mutex mutex_;
};

class MultiThreadedCommandBuffer {
public:
    MultiThreadedCommandBuffer();
    ~MultiThreadedCommandBuffer();
    
    void Initialize(size_t threadCount, size_t commandsPerThread);
    void Shutdown();
    
    void BeginFrame();
    void EndFrame();
    
    CommandBuffer* GetThreadBuffer(size_t threadIndex);
    CommandBuffer* GetCurrentThreadBuffer();
    
    size_t GetTotalCommandCount() const;
    size_t GetThreadCount() const { return threadBuffers_.size(); }
    
    void SetEnabled(bool enabled) { enabled_ = enabled; }
    bool IsEnabled() const { return enabled_; }

private:
    std::vector<std::unique_ptr<CommandBuffer>> threadBuffers_;
    bool enabled_ = true;
    bool initialized_ = false;
    uint32_t currentFrame_;
    
    static thread_local size_t currentThreadIndex_;
};

} // namespace renderer
} // namespace ge
