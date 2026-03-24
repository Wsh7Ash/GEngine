#include "RenderSystem.h"
#include "../../math/Mat4x4.h"
#include "../../renderer/Mesh.h"
#include "../../renderer/Renderer2D.h"
#include "../../renderer/Shader.h"
#include "../../renderer/Texture.h"

namespace ge {
namespace ecs {

void RenderSystem::Render(World &world, float dt) {
  // 1. 3D Pass (MeshComponents)
  for (auto const &entity : entities) {
    if (world.HasComponent<MeshComponent>(entity) &&
        !world.HasComponent<SpriteComponent>(entity)) {
      auto &transform = world.GetComponent<TransformComponent>(entity);
      auto &meshComp = world.GetComponent<MeshComponent>(entity);
      if (meshComp.MeshPtr && meshComp.ShaderPtr) {
        meshComp.ShaderPtr->Bind();
        Math::Mat4f model = Math::Mat4f::Translate(transform.position) *
                            transform.rotation.ToMat4x4() *
                            Math::Mat4f::Scale(transform.scale);

        meshComp.ShaderPtr->SetMat4("u_Model", model);
        meshComp.MeshPtr->Draw();
      }
    }
  }

  // 2. 2D Batch Pass (SpriteComponents)
  if (camera2D_) {
    renderer::Renderer2D::BeginScene(*camera2D_);
    for (auto const &entity : entities) {
      if (world.HasComponent<SpriteComponent>(entity)) {
        auto &transform = world.GetComponent<TransformComponent>(entity);
        auto &spriteComp = world.GetComponent<SpriteComponent>(entity);

        // Animation properties (updated by AnimationSystem)
        Math::Vec2f uvTiling = spriteComp.tiling;
        Math::Vec2f uvOffset = {0.0f, 0.0f};

        if (spriteComp.framesX > 0 && spriteComp.framesY > 0) {
          float frameWidth = 1.0f / (float)spriteComp.framesX;
          float frameHeight = 1.0f / (float)spriteComp.framesY;
          uvTiling = {frameWidth, frameHeight};

          int column = spriteComp.currentFrame % spriteComp.framesX;
          int row = spriteComp.currentFrame / spriteComp.framesX;

          uvOffset = {(float)column * frameWidth,
                      (float)(spriteComp.framesY - 1 - row) * frameHeight};
        }

        Math::Vec2f size = {transform.scale.x, transform.scale.y};
        if (spriteComp.texture)
          renderer::Renderer2D::DrawQuad(
              transform.position, size, spriteComp.texture, spriteComp.color,
              (int)entity.GetIndex(), spriteComp.FlipX, spriteComp.FlipY,
              uvTiling, uvOffset);
        else
          renderer::Renderer2D::DrawQuad(transform.position, size,
                                         spriteComp.color,
                                         (int)entity.GetIndex());
      }
    }
    renderer::Renderer2D::EndScene();
  }
}

} // namespace ecs
} // namespace ge
