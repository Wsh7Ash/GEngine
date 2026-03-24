#pragma once

#include "../../renderer/OrthographicCamera.h"
#include "../../renderer/PerspectiveCamera.h"
#include "../System.h"
#include "../World.h"
#include "../components/MeshComponent.h"
#include "../components/SpriteComponent.h"
#include "../components/TransformComponent.h"
#include <memory>


namespace ge {
namespace renderer {
class Mesh;
class Shader;
class Framebuffer;
} // namespace renderer

namespace ecs {

/**
 * @brief System responsible for rendering all Entities with a MeshComponent and
 * TransformComponent.
 */
class RenderSystem : public System {
public:
  void Render(World &world, float dt = 0.0f);

  void
  Set2DCamera(const std::shared_ptr<renderer::OrthographicCamera> &camera) {
    camera2D_ = camera;
  }

  void
  Set3DCamera(const std::shared_ptr<renderer::PerspectiveCamera> &camera) {
    camera3D_ = camera;
  }

private:
  std::shared_ptr<renderer::OrthographicCamera> camera2D_;
  std::shared_ptr<renderer::PerspectiveCamera> camera3D_;
  
  std::shared_ptr<renderer::Shader> shadowShader_;
  std::shared_ptr<renderer::Framebuffer> shadowMap_;

  // IBL Helpers
  void SetupEnvironment(struct SkyboxComponent& skybox);
  void RenderCube();
  void RenderQuad();
};

} // namespace ecs
} // namespace ge
