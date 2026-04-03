#pragma once

// ================================================================
//  PassLibrary.h
//  Common render pass implementations.
// ================================================================

#include "RenderPass.h"
#include "TextureHandle.h"
#include "../Shader.h"
#include "../Cubemap.h"
#include <vector>
#include <memory>
#include <cstdint>

namespace ge {
namespace renderer {

struct GBufferTargets {
    TextureHandle position;
    TextureHandle normal;
    TextureHandle albedo;
    TextureHandle metallicRoughness;
    TextureHandle emissive;
    TextureHandle depth;
};

class GBufferPass : public RenderPass {
public:
    GBufferPass(const std::string& name = "GBuffer");
    ~GBufferPass() override = default;
    
    void Initialize(RenderGraph& graph) override;
    void Execute(RenderGraph& graph, const PassProfile& profile) override;
    void OnResize(uint32_t width, uint32_t height) override;
    
    GBufferTargets& GetTargets() { return targets_; }
    const GBufferTargets& GetTargets() const { return targets_; }
    
private:
    GBufferTargets targets_;
    std::shared_ptr<Shader> gBufferShader_;
};

struct SSAOTargets {
    TextureHandle ssao;
    TextureHandle blurH;
    TextureHandle blurV;
};

class SSAOPass : public RenderPass {
public:
    SSAOPass(const std::string& name = "SSAO");
    ~SSAOPass() override = default;
    
    void Initialize(RenderGraph& graph) override;
    void Execute(RenderGraph& graph, const PassProfile& profile) override;
    void OnResize(uint32_t width, uint32_t height) override;
    
    void SetRadius(float radius) { radius_ = radius; }
    void SetBias(float bias) { bias_ = bias; }
    void SetIntensity(float intensity) { intensity_ = intensity; }
    void SetKernelSize(int size) { kernelSize_ = size; }
    
    SSAOTargets& GetTargets() { return targets_; }
    const SSAOTargets& GetTargets() const { return targets_; }
    
private:
    SSAOTargets targets_;
    std::shared_ptr<Shader> ssaoShader_;
    std::shared_ptr<Shader> blurShader_;
    
    float radius_ = 0.5f;
    float bias_ = 0.025f;
    float intensity_ = 1.0f;
    int kernelSize_ = 64;
    std::vector<Math::Vec3f> kernel_;
    uint32_t noiseTexture_ = 0;
};

class DeferredLightingPass : public RenderPass {
public:
    DeferredLightingPass(const std::string& name = "DeferredLighting");
    ~DeferredLightingPass() override = default;
    
    void Initialize(RenderGraph& graph) override;
    void Execute(RenderGraph& graph, const PassProfile& profile) override;
    void OnResize(uint32_t width, uint32_t height) override;
    
    void SetAmbientColor(const Math::Vec3f& color) { ambientColor_ = color; }
    void SetAmbientIntensity(float intensity) { ambientIntensity_ = intensity; }
    void SetSkyboxEnabled(bool enabled) { useSkybox_ = enabled; }
    
    TextureHandle& GetOutput() { return output_; }
    
private:
    TextureHandle output_;
    std::shared_ptr<Shader> deferredShader_;
    
    Math::Vec3f ambientColor_ = {0.03f, 0.03f, 0.03f};
    float ambientIntensity_ = 1.0f;
    bool useSkybox_ = true;
};

class ForwardPass : public RenderPass {
public:
    ForwardPass(const std::string& name = "Forward");
    ~ForwardPass() override = default;
    
    void Initialize(RenderGraph& graph) override;
    void Execute(RenderGraph& graph, const PassProfile& profile) override;
    void OnResize(uint32_t width, uint32_t height) override;
    
    void AddLight(const Math::Vec3f& position, const Math::Vec3f& color, float intensity);
    void ClearLights();
    
private:
    struct Light {
        Math::Vec3f position;
        Math::Vec3f color;
        float intensity;
    };
    
    TextureHandle output_;
    std::shared_ptr<Shader> forwardShader_;
    std::vector<Light> lights_;
};

class ShadowPass : public RenderPass {
public:
    ShadowPass(const std::string& name = "Shadow");
    ~ShadowPass() override = default;
    
    void Initialize(RenderGraph& graph) override;
    void Execute(RenderGraph& graph, const PassProfile& profile) override;
    void OnResize(uint32_t width, uint32_t height) override;
    
    void SetLightPosition(const Math::Vec3f& position) { lightPosition_ = position; }
    void SetLightDirection(const Math::Vec3f& direction) { lightDirection_ = direction; }
    void SetShadowMapSize(uint32_t size) { shadowMapSize_ = size; }
    
    TextureHandle& GetShadowMap() { return shadowMap_; }
    
private:
    TextureHandle shadowMap_;
    std::shared_ptr<Shader> shadowShader_;
    
    Math::Vec3f lightPosition_ = {10.0f, 10.0f, 10.0f};
    Math::Vec3f lightDirection_ = {-1.0f, -1.0f, -1.0f};
    Math::Mat4f lightViewProj_;
    uint32_t shadowMapSize_ = 2048;
};

class PostProcessPass : public RenderPass {
public:
    PostProcessPass(const std::string& name = "PostProcess");
    ~PostProcessPass() override = default;
    
    void Initialize(RenderGraph& graph) override;
    void Execute(RenderGraph& graph, const PassProfile& profile) override;
    void OnResize(uint32_t width, uint32_t height) override;
    
    enum class Effect {
        None,
        Bloom,
        ToneMapping,
        GammaCorrection,
        Vignette,
        ChromaticAberration,
        FXAA,
        TAA
    };
    
    void AddEffect(Effect effect);
    void RemoveEffect(Effect effect);
    bool HasEffect(Effect effect) const;
    void ClearEffects();
    
    TextureHandle& GetInput() { return input_; }
    TextureHandle& GetOutput() { return output_; }
    
private:
    void ExecuteBloom();
    void ExecuteToneMapping();
    void ExecuteGammaCorrection();
    void ExecuteVignette();
    void ExecuteFXAA();
    
    TextureHandle input_;
    TextureHandle output_;
    TextureHandle bloomBufferA_;
    TextureHandle bloomBufferB_;
    
    std::vector<Effect> effects_;
    std::shared_ptr<Shader> postProcessShader_;
    
    struct BloomSettings {
        float threshold = 0.8f;
        float intensity = 0.5f;
        int iterations = 5;
    } bloomSettings_;
};

class SkyboxPass : public RenderPass {
public:
    SkyboxPass(const std::string& name = "Skybox");
    ~SkyboxPass() override = default;
    
    void Initialize(RenderGraph& graph) override;
    void Execute(RenderGraph& graph, const PassProfile& profile) override;
    void OnResize(uint32_t width, uint32_t height) override;
    
    void SetCubemap(std::shared_ptr<Cubemap> cubemap) { cubemap_ = cubemap; }
    std::shared_ptr<Cubemap> GetCubemap() const { return cubemap_; }
    
    TextureHandle& GetOutput() { return output_; }
    
private:
    TextureHandle output_;
    std::shared_ptr<Shader> skyboxShader_;
    std::shared_ptr<Cubemap> cubemap_;
};

class ResolvePass : public RenderPass {
public:
    ResolvePass(const std::string& name = "Resolve");
    ~ResolvePass() override = default;
    
    void Initialize(RenderGraph& graph) override;
    void Execute(RenderGraph& graph, const PassProfile& profile) override;
    
    void SetInput(const std::string& name) { inputName_ = name; }
    void SetOutput(const std::string& name) { outputName_ = name; }
    
private:
    std::string inputName_;
    std::string outputName_;
};

class PresentPass : public RenderPass {
public:
    PresentPass(const std::string& name = "Present");
    ~PresentPass() override = default;
    
    void Initialize(RenderGraph& graph) override;
    void Execute(RenderGraph& graph, const PassProfile& profile) override;
    
    void SetInput(const std::string& name) { inputName_ = name; }
    
private:
    std::string inputName_;
};

} // namespace renderer
} // namespace ge
