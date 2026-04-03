#pragma once

#include "../../renderer/OrthographicCamera.h"
#include "../../renderer/PerspectiveCamera.h"
#include "../../renderer/graph/RenderGraph.h"
#include "../System.h"
#include "../World.h"
#include "../components/MeshComponent.h"
#include "../components/SpriteComponent.h"
#include "../components/TransformComponent.h"
#include "../components/DecalComponent.h"
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace ge {
namespace renderer {
    class Mesh;
    class Shader;
    class PostProcessingStack;
    class Framebuffer;
}

struct PostProcessingSettings {
    bool EnableSSAO = true;
    float SSAORadius = 0.5f;
    float SSAOBias = 0.025f;
    float SSAOIntensity = 1.0f;
    int SSAOKernelSize = 64;

    bool EnableVolumetric = true;
    float VolumetricScattering = 0.5f;
    float VolumetricIntensity = 1.0f;
    int VolumetricSamples = 32;

    bool EnableTAA = true;
    
    // SSGI Settings
    bool EnableSSGI = false;  // Disabled by default for performance
    float SSGIRadius = 0.5f;
    int SSGISampleCount = 16;
    float SSGIIntensity = 1.0f;
    float SSGIBounceIntensity = 0.5f; // For approximating multiple bounces

    // SSR Settings
    bool EnableSSR = false;   // Disabled by default for performance
    int SSRSteps = 32;
    float SSRStepSize = 0.5;
    float SSRFadeDistance = 10.0;
    float SSRThickness = 0.01;
    float SSRRoughnessFade = 1.0;
    float SSRIntensity = 1.0;  // SSR intensity multiplier

    // Plasma/Glow Settings
    bool EnablePlasma = false;
    float PlasmaSpeed = 1.0f;
    float PlasmaIntensity = 0.5f;
    float PlasmaScale = 2.0f;
    Math::Vec3f PlasmaColorA = {1.0f, 0.5f, 0.0f};  // Orange
    Math::Vec3f PlasmaColorB = {0.0f, 0.5f, 1.0f};  // Blue
    Math::Vec3f PlasmaColorC = {1.0f, 0.0f, 1.0f};  // Magenta

    // SSS (Subsurface Scattering) Settings
    bool EnableSSSS = false;
    float SSSSPower = 12.0f;
    float SSSSRadius = 0.5f;
    float SSSSIntensity = 1.0f;
    int SSSSampleCount = 16;

    // Refraction Settings
    bool EnableRefraction = false;
    float RefractionIntensity = 1.0f;

    // Forward+ Settings
    bool EnableForwardPlus = false;
    int ClusterSizeX = 16;
    int ClusterSizeY = 8;
    int ClusterSizeZ = 16;
    int MaxLightsPerCluster = 16;
    int MaxLights = 256;

    // Render Graph Settings
    bool EnableRenderGraph = false;
};

namespace ecs {

/**
 * @brief System responsible for rendering all Entities with a MeshComponent and
 * TransformComponent.
 */
class RenderSystem : public System {
public:
  void Render(World &world, float dt = 0.0f);
  void ExecuteSSAOPass(World& world);
  void ExecuteCSMPass(World& world);
  void ExecuteVolumetricPass(World& world);
  void ExecutePlasmaPass(World& world);
  void ExecuteSSSSPass(World& world);
  void ExecuteRefractionPass(World& world);

  void
  Set2DCamera(const std::shared_ptr<renderer::OrthographicCamera> &camera) {
    camera2D_ = camera;
  }

  void
  Set3DCamera(const std::shared_ptr<renderer::PerspectiveCamera> &camera) {
    camera3D_ = camera;
  }

  PostProcessingSettings& GetSettings() { return settings_; }

private:
  std::shared_ptr<renderer::OrthographicCamera> camera2D_;
  std::shared_ptr<renderer::PerspectiveCamera> camera3D_;
  
    std::shared_ptr<renderer::Shader> shadowShader_;
  std::shared_ptr<renderer::Framebuffer> shadowMap_;
  
  // CSM (Cascaded Shadow Maps)
  static constexpr int MAX_CSM_CASCADES = 4;
  std::vector<std::shared_ptr<renderer::Framebuffer>> csmFramebuffers_;
  std::vector<Math::Mat4f> csmLightSpaceMatrices_;
  std::vector<float> csmSplitDepths_;
  int csmCount_ = 4;
  int csmShadowMapSize_ = 2048;
  float csmShadowBias_ = 0.001f;
  float csmBlendWidth_ = 0.1f;
  
  // Volumetric Fog
  bool volumetricFogEnabled_ = false;
  float fogDensity_ = 0.05f;
  float fogHeight_ = 0.0f;
  float fogHeightFalloff_ = 1.0f;
  float fogAnisotropy_ = 0.5f;
  float fogMultiScattering_ = 0.5f;
  Math::Vec3f fogColor_ = { 0.7f, 0.8f, 0.9f };
  float fogStartDistance_ = 1.0f;

    // SSAO
    std::shared_ptr<renderer::Shader> ssaoShader_, ssaoBlurShader_, gBufferShader_;
    std::shared_ptr<renderer::Framebuffer> gBuffer_, ssaoFBO_, ssaoBlurFBO_;
    
    // SSGI
    std::shared_ptr<renderer::Shader> ssgiShader_;
    std::shared_ptr<renderer::Framebuffer> ssgiFBO_;
    
    // SSR
    std::shared_ptr<renderer::Shader> ssrShader_;
    std::shared_ptr<renderer::Framebuffer> ssrFBO_;

    // Plasma
    std::shared_ptr<renderer::Shader> plasmaShader_;

    // SSS (Subsurface Scattering)
    std::shared_ptr<renderer::Shader> ssssShader_;
    std::shared_ptr<renderer::Framebuffer> ssssFBO_;

    // Refraction
    std::shared_ptr<renderer::Shader> refractionShader_;
    std::shared_ptr<renderer::Framebuffer> refractionFBO_;

    // Forward+ Lighting
    std::shared_ptr<renderer::Shader> forwardPlusShader_;
    unsigned int lightBuffer_ = 0;          // SSBO for light data
    unsigned int clusterLightIndicesBuffer_ = 0;  // SSBO for cluster light indices
    unsigned int clusterLightCountsBuffer_ = 0;   // SSBO for light count per cluster
    unsigned int clusterDataTexture_ = 0;  // 3D texture for cluster bounds
    unsigned int lightIndicesTexture_ = 0; // 2D texture for light indices per cluster
    int clusterCountX_ = 16;
    int clusterCountY_ = 8;
    int clusterCountZ_ = 16;
    int maxLightsPerCluster_ = 16;
    int maxLights_ = 256;

    struct ForwardPlusData {
        Math::Vec3f* lightPositions;
        Math::Vec3f* lightColors;
        float* lightIntensities;
        float* lightRanges;
        int* lightTypes;
        Math::Vec3f* lightDirections;
        float* lightSpotOuter;
        float* lightSpotInner;
        int* lightIndicesPerCluster;
        int* lightCountPerCluster;
        Math::Vec4f* clusterBounds;
    };
    ForwardPlusData fpData_;

    void BuildForwardPlusClusters();
    void AssignLightsToClusters(const std::vector<ecs::Entity>& lightEntities, World& world);
    void UploadLightDataToGPU(int lightCount);

   // Decal
   std::shared_ptr<renderer::Shader> decalShader_;

   // Volumetric Lighting
  std::shared_ptr<renderer::Shader> volumetricShader_;
  std::shared_ptr<renderer::Framebuffer> volumetricFBO_;
  std::vector<Math::Vec3f> ssaoKernel_;
  unsigned int ssaoNoiseTexture_ = 0;

  // IBL Helpers
  void SetupEnvironment(struct SkyboxComponent& skybox);
  void RenderCube();
  void RenderQuad();

  std::shared_ptr<renderer::PostProcessingStack> postProcessingStack_;
  std::shared_ptr<renderer::Framebuffer> intermediateA_;
  std::shared_ptr<renderer::Framebuffer> intermediateB_;
  
  // Render Graph (optional)
  std::shared_ptr<renderer::RenderGraph> renderGraph_;
  
  // TAA / Temporal Tracking
  Math::Mat4f prevViewProj_;
  std::unordered_map<ecs::Entity, Math::Mat4f> prevModelMatrices_;
  unsigned int frameIndex_ = 0;
  Math::Vec2f jitter_ = {0, 0};
  std::shared_ptr<renderer::Shader> taaShader_;
  std::shared_ptr<renderer::Framebuffer> prevFrameFBO_, resolveFBO_;

  PostProcessingSettings settings_;
};

} // namespace ecs
} // namespace ge
