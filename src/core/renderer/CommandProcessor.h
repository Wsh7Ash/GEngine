#pragma once

// ================================================================
//  CommandProcessor.h
//  Main thread command processor for executing draw commands.
// ================================================================

#include "CommandBuffer.h"
#include "RenderQueue.h"
#include <functional>
#include <vector>
#include <atomic>

namespace ge {
namespace renderer {

class Mesh;
class Material;
class Shader;
class Framebuffer;

struct ProcessorStatistics {
    uint32_t drawCalls = 0;
    uint32_t instancedDrawCalls = 0;
    uint32_t totalInstances = 0;
    uint32_t culledCommands = 0;
    uint32_t executedCommands = 0;
    float processTime = 0.0f;
    float sortTime = 0.0f;
    
    void Reset() {
        drawCalls = 0;
        instancedDrawCalls = 0;
        totalInstances = 0;
        culledCommands = 0;
        executedCommands = 0;
        processTime = 0.0f;
        sortTime = 0.0f;
    }
};

class CommandProcessor {
public:
    CommandProcessor();
    ~CommandProcessor();
    
    void Initialize();
    void Shutdown();
    
    void BeginFrame();
    void EndFrame();
    
    void Process(const RenderQueue& queue);
    void ProcessOpaque(const std::vector<DrawCommand>& commands);
    void ProcessTransparent(const std::vector<DrawCommand>& commands);
    void ProcessUI(const std::vector<DrawCommand>& commands);
    void ProcessShadows(const std::vector<DrawCommand>& commands);
    
    void SetCurrentFramebuffer(Framebuffer* fbo) { currentFramebuffer_ = fbo; }
    Framebuffer* GetCurrentFramebuffer() const { return currentFramebuffer_; }
    
    void SetCurrentCamera(const void* camera) { currentCamera_ = camera; }
    const void* GetCurrentCamera() const { return currentCamera_; }
    
    void SetViewMatrix(const Math::Mat4f& view) { viewMatrix_ = view; }
    void SetProjectionMatrix(const Math::Mat4f& projection) { projectionMatrix_ = projection; }
    
    const ProcessorStatistics& GetStats() const { return stats_; }
    ProcessorStatistics& GetStats() { return stats_; }
    
    void SetEnabled(bool enabled) { enabled_ = enabled; }
    bool IsEnabled() const { return enabled_; }
    
    using DrawCallback = std::function<void(const DrawCommand&)>;
    void SetDrawCallback(DrawCallback callback) { drawCallback_ = callback; }

private:
    void ExecuteCommand(const DrawCommand& cmd);
    void ExecuteMesh(const DrawCommand& cmd);
    void ExecuteInstanced(const DrawCommand& cmd);
    void ExecuteFullscreen(const DrawCommand& cmd);
    void ExecuteSkybox(const DrawCommand& cmd);
    
    void BindState(const DrawCommand& cmd);
    void UnbindState();
    
    ProcessorStatistics stats_;
    
    Framebuffer* currentFramebuffer_ = nullptr;
    const void* currentCamera_ = nullptr;
    
    Math::Mat4f viewMatrix_;
    Math::Mat4f projectionMatrix_;
    
    uint32_t currentMaterialID_ = 0;
    uint32_t currentMeshID_ = 0;
    uint32_t currentShaderID_ = 0;
    
    bool enabled_ = true;
    bool stateDirty_ = true;
    
    DrawCallback drawCallback_;
};

class ParallelCuller {
public:
    ParallelCuller();
    ~ParallelCuller();
    
    void Initialize(size_t threadCount);
    void Shutdown();
    
    struct CullingResult {
        std::vector<DrawCommand> visibleCommands;
        uint32_t culledCount = 0;
        uint32_t visibleCount = 0;
    };
    
    void BeginFrame();
    void EndFrame();
    
    void CullFrustum(const std::vector<DrawCommand>& input, CullingResult& output,
                     const Math::Mat4f& viewProj, const Math::Vec3f& cameraPos);
    
    void CullDistance(const std::vector<DrawCommand>& input, CullingResult& output,
                      float maxDistance, const Math::Vec3f& cameraPos);
    
    void CullOcclusion(const std::vector<DrawCommand>& input, CullingResult& output);
    
    size_t GetThreadCount() const { return threadCount_; }

private:
    void WorkerThreadFunction(size_t threadIndex);
    
    size_t threadCount_ = 0;
    std::vector<std::thread> workerThreads_;
    std::atomic<bool> shutdown_{false};
    std::atomic<bool> workAvailable_{false};
    std::atomic<bool> workComplete_{false};
    
    std::mutex inputMutex_;
    std::mutex outputMutex_;
    
    std::vector<std::vector<DrawCommand>*> threadInputs_;
    std::vector<CullingResult*> threadOutputs_;
};

} // namespace renderer
} // namespace ge
