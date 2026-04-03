#pragma once

// ================================================================
//  GraphBuilder.h
//  Fluent API for building render graphs.
// ================================================================

#include "RenderGraph.h"
#include "PassLibrary.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace ge {
namespace renderer {

class GraphBuilder {
public:
    explicit GraphBuilder(RenderGraph& graph);
    ~GraphBuilder() = default;
    
    GraphBuilder& SetName(const std::string& name);
    
    template<typename PassType>
    GraphBuilder& AddPass(const std::string& name) {
        pass_ = graph_.AddPass<PassType>(name);
        currentPassName_ = name;
        return *this;
    }
    
    GraphBuilder& WithInput(const std::string& name);
    GraphBuilder& WithOutput(const std::string& name, const TextureDesc& desc);
    GraphBuilder& DependsOn(const std::string& passName);
    GraphBuilder& DependsOn(const std::vector<std::string>& passNames);
    GraphBuilder& Enabled(bool enabled);
    GraphBuilder& Timing(PassTiming timing);
    GraphBuilder& Mode(PassExecutionMode mode);
    
    GraphBuilder& OnExecute(std::function<void()> callback);
    
    GraphBuilder& AddGBufferPass();
    GraphBuilder& AddSSAOPass();
    GraphBuilder& AddDeferredLightingPass();
    GraphBuilder& AddForwardPass();
    GraphBuilder& AddShadowPass();
    GraphBuilder& AddPostProcessPass();
    GraphBuilder& AddSkyboxPass();
    GraphBuilder& AddPresentPass();
    
    RenderGraph& Build();
    
    RenderPass* GetCurrentPass() const { return pass_; }
    const std::string& GetCurrentPassName() const { return currentPassName_; }
    
private:
    RenderGraph& graph_;
    RenderPass* pass_ = nullptr;
    std::string currentPassName_;
    std::string graphName_;
};

class RenderGraphFactory {
public:
    static std::unique_ptr<RenderGraph> CreateDeferred(const GraphSettings& settings = GraphSettings{});
    static std::unique_ptr<RenderGraph> CreateForward(const GraphSettings& settings = GraphSettings{});
    static std::unique_ptr<RenderGraph> CreateForwardPlus(const GraphSettings& settings = GraphSettings{});
    static std::unique_ptr<RenderGraph> CreateMobile(const GraphSettings& settings = GraphSettings{});
    
private:
    static void SetupDeferredGraph(GraphBuilder& builder);
    static void SetupForwardGraph(GraphBuilder& builder);
    static void SetupForwardPlusGraph(GraphBuilder& builder);
    static void SetupMobileGraph(GraphBuilder& builder);
};

class GraphSetup {
public:
    GraphSetup() = default;
    ~GraphSetup() = default;
    
    GraphSetup& EnableSSAO(bool enable = true);
    GraphSetup& EnableSSR(bool enable = true);
    GraphSetup& EnableBloom(bool enable = true);
    GraphSetup& EnableToneMapping(bool enable = true);
    GraphSetup& EnableShadowCascades(bool enable = true, int cascades = 4);
    GraphSetup& EnableTAA(bool enable = true);
    
    GraphSetup& SetShadowQuality(int quality);
    GraphSetup& SetTextureQuality(int quality);
    GraphSetup& SetResolutionScale(float scale);
    
    GraphSettings ToSettings() const;
    
private:
    bool enableSSAO_ = true;
    bool enableSSR_ = false;
    bool enableBloom_ = true;
    bool enableToneMapping_ = true;
    bool enableShadowCascades_ = true;
    bool enableTAA_ = false;
    int shadowCascades_ = 4;
    int shadowQuality_ = 2;
    int textureQuality_ = 2;
    float resolutionScale_ = 1.0f;
};

} // namespace renderer
} // namespace ge
