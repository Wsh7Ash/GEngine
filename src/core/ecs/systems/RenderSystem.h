#pragma once

#include "../../renderer/OrthographicCamera.h"
#include "../System.h"
#include "../World.h"
#include "../components/MeshComponent.h"
#include "../components/SpriteComponent.h"
#include "../components/TransformComponent.h"
#include <memory>


namespace ge {
namespace renderer {
class Mesh;
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

private:
  std::shared_ptr<renderer::OrthographicCamera> camera2D_;
};

} // namespace ecs
} // namespace ge
