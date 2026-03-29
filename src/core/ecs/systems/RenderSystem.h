#pragma once

#include "../../renderer/OrthographicCamera.h"
#include "../../renderer/PerspectiveCamera.h"
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
  void ExecuteVolumetricPass(World& world);

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

   // SSAO
   std::shared_ptr<renderer::Shader> ssaoShader_, ssaoBlurShader_, gBufferShader_;
   std::shared_ptr<renderer::Framebuffer> gBuffer_, ssaoFBO_, ssaoBlurFBO_;

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
