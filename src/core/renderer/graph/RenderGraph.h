#pragma once

// ================================================================
//  RenderGraph.h
//  Main render graph container and API.
// ================================================================

#include "RenderPass.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <functional>
#include <cstdint>

namespace ge {
namespace renderer {

class GraphExecutor;

struct GraphStatistics {
    size_t totalPasses = 0;
    size_t totalTextures = 0;
    size_t totalTextureMemory = 0;
    size_t transientTextureMemory = 0;
    float totalGpuTimeMs = 0.0f;
    int maxDrawCalls = 0;
};

struct GraphSettings {
    bool autoInsertBarriers = true;
    bool validateGraph = true;
    bool enableProfiling = true;
    bool enableStatistics = true;
    int maxRenderTargets = 8;
    uint32_t defaultWidth = 1920;
    uint32_t defaultHeight = 1080;
};

class RenderGraph {
public:
    RenderGraph();
    ~RenderGraph();
    
    void Initialize();
    void Shutdown();
    
    void Reset();
    
    template<typename T>
    T* AddPass(const std::string& name) {
        static_assert(std::is_base_of_v<RenderPass, T>, "T must derive from RenderPass");
        auto pass = std::make_unique<T>(name);
        pass->Initialize(*this);
        T* ptr = pass.get();
        passes_[name] = std::move(pass);
        passOrder_.push_back(name);
        return ptr;
    }
    
    RenderPass* AddPass(std::unique_ptr<RenderPass> pass);
    void RemovePass(const std::string& name);
    
    RenderPass* GetPass(const std::string& name);
    const RenderPass* GetPass(const std::string& name) const;
    
    bool HasPass(const std::string& name) const;
    std::vector<std::string> GetPassNames() const;
    
    TextureHandle& CreateTexture(const std::string& name, const TextureDesc& desc);
    TextureHandle* GetTexture(const std::string& name);
    const TextureHandle* GetTexture(const std::string& name) const;
    bool HasTexture(const std::string& name) const;
    void RemoveTexture(const std::string& name);
    
    void AddDependency(const std::string& fromPass, const std::string& toPass);
    void RemoveDependency(const std::string& fromPass, const std::string& toPass);
    
    bool Validate();
    std::vector<std::string> GetValidationErrors() const;
    std::vector<std::string> GetValidationWarnings() const;
    
    void Build();
    void Execute();
    void Execute(uint32_t width, uint32_t height);
    
    void Resize(uint32_t width, uint32_t height);
    uint32_t GetWidth() const { return width_; }
    uint32_t GetHeight() const { return height_; }
    
    const GraphStatistics& GetStatistics() const { return stats_; }
    GraphStatistics& GetStatistics() { return stats_; }
    
    void SetSettings(const GraphSettings& settings);
    const GraphSettings& GetSettings() const { return settings_; }
    GraphSettings& GetSettings() { return settings_; }
    
    std::shared_ptr<Framebuffer> GetWindowFramebuffer() const { return windowFramebuffer_; }
    void SetWindowFramebuffer(std::shared_ptr<Framebuffer> fb);
    
    std::function<void()> onExecuteBegin;
    std::function<void()> onExecuteEnd;
    
    static std::unique_ptr<RenderGraph> Create();
    static std::unique_ptr<RenderGraph> CreateDefault();
    
private:
    void CalculatePassOrder();
    void SetupTextureLinks();
    void UpdateStatistics();
    void CleanupTransientTextures();
    
    std::unordered_map<std::string, std::unique_ptr<RenderPass>> passes_;
    std::vector<std::string> passOrder_;
    std::unordered_map<std::string, TextureHandle> textures_;
    
    std::vector<std::pair<std::string, std::string>> explicitDependencies_;
    std::unordered_set<std::string> transientTextures_;
    
    std::unique_ptr<GraphExecutor> executor_;
    std::shared_ptr<Framebuffer> windowFramebuffer_;
    
    GraphSettings settings_;
    GraphStatistics stats_;
    
    uint32_t width_ = 1920;
    uint32_t height_ = 1080;
    bool isBuilt_ = false;
    bool isExecuting_ = false;
    
    std::vector<std::string> validationErrors_;
    std::vector<std::string> validationWarnings_;
    
    friend class RenderPass;
    friend class GraphExecutor;
};

} // namespace renderer
} // namespace ge
