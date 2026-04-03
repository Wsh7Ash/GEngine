#pragma once

// ================================================================
//  RenderPass.h
//  Base pass interface for render graph.
// ================================================================

#include "TextureHandle.h"
#include "../Shader.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <cstdint>

namespace ge {
namespace renderer {

class RenderGraph;

enum class PassExecutionMode {
    Serial,
    Parallel
};

enum class PassTiming {
    Early,
    Normal,
    Late
};

struct PassDependency {
    std::string passName;
    std::string outputName;
    std::string inputName;
};

struct PassProfile {
    float gpuTimeMs = 0.0f;
    int drawCalls = 0;
    int triangles = 0;
    int samplesWritten = 0;
    int samplesRead = 0;
};

class RenderPass {
public:
    virtual ~RenderPass() = default;
    
    virtual void Initialize(RenderGraph& graph) {}
    virtual void Execute(RenderGraph& graph, const PassProfile& profile) = 0;
    virtual void Shutdown(RenderGraph& graph) {}
    
    virtual void OnResize(uint32_t width, uint32_t height) {}
    
    const std::string& GetName() const { return name_; }
    void SetName(const std::string& name) { name_ = name; }
    
    PassExecutionMode GetExecutionMode() const { return executionMode_; }
    void SetExecutionMode(PassExecutionMode mode) { executionMode_ = mode; }
    
    PassTiming GetTiming() const { return timing_; }
    void SetTiming(PassTiming timing) { timing_ = timing; }
    
    const std::vector<PassDependency>& GetDependencies() const { return dependencies_; }
    const std::vector<std::string>& GetInputs() const { return inputNames_; }
    const std::vector<std::string>& GetOutputs() const { return outputNames_; }
    
    bool IsEnabled() const { return enabled_; }
    void SetEnabled(bool enabled) { enabled_ = enabled; }
    
    bool HasInput(const std::string& name) const;
    bool HasOutput(const std::string& name) const;
    
    TextureHandle* GetInputTexture(const std::string& name);
    TextureHandle* GetOutputTexture(const std::string& name);
    
    const PassProfile& GetProfile() const { return profile_; }
    PassProfile& GetProfile() { return profile_; }
    
    std::function<void()> onExecute;
    
protected:
    RenderPass(const std::string& name);
    
    void AddInput(const std::string& name);
    void AddOutput(const std::string& name, const TextureDesc& desc);
    void AddDependency(const std::string& passName, const std::string& outputName, const std::string& inputName);
    
    void RequireInput(const std::string& name);
    void RequireOutput(const std::string& name, const TextureDesc& desc);
    
    std::string name_;
    bool enabled_ = true;
    PassExecutionMode executionMode_ = PassExecutionMode::Serial;
    PassTiming timing_ = PassTiming::Normal;
    
    std::vector<std::string> inputNames_;
    std::vector<std::string> outputNames_;
    std::vector<PassDependency> dependencies_;
    
    std::unordered_map<std::string, TextureHandle> inputTextures_;
    std::unordered_map<std::string, TextureHandle> outputTextures_;
    
    PassProfile profile_;
    
    friend class RenderGraph;
    friend class GraphExecutor;
};

class ComputePass : public RenderPass {
public:
    ComputePass(const std::string& name);
    ~ComputePass() override = default;
    
    void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
    
    void SetShader(std::shared_ptr<Shader> shader) { computeShader_ = shader; }
    std::shared_ptr<Shader> GetShader() const { return computeShader_; }
    
    void Execute(RenderGraph& graph, const PassProfile& profile) override;
    
private:
    std::shared_ptr<Shader> computeShader_;
    uint32_t dispatchX_ = 1;
    uint32_t dispatchY_ = 1;
    uint32_t dispatchZ_ = 1;
};

class CopyPass : public RenderPass {
public:
    CopyPass(const std::string& name);
    ~CopyPass() override = default;
    
    void Copy(const std::string& source, const std::string& dest);
    void Blit(const std::string& source, const std::string& dest);
    
    void Execute(RenderGraph& graph, const PassProfile& profile) override;
    
private:
    std::vector<std::pair<std::string, std::string>> copyCommands_;
};

} // namespace renderer
} // namespace ge
