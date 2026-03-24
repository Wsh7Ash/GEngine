#include <glad/glad.h>
#include "RenderSystem.h"
#include "../../math/Mat4x4.h"
#include "../../renderer/Material.h"
#include "../../renderer/Mesh.h"
#include "../../renderer/Renderer2D.h"
#include "../../renderer/Shader.h"
#include "../../renderer/Texture.h"
#include "../../renderer/PerspectiveCamera.h"
#include "../../renderer/Framebuffer.h"
#include "../components/LightComponent.h"

namespace ge {
namespace ecs {

void RenderSystem::Render(World &world, float dt) {
  // 1. Setup Shadow System (Lazy Init)
  if (!shadowMap_) {
      FramebufferSpecification shadowSpec;
      shadowSpec.Width = 2048;
      shadowSpec.Height = 2048;
      shadowSpec.Attachments = { FramebufferTextureFormat::DEPTH24STENCIL8 };
      shadowMap_ = Framebuffer::Create(shadowSpec);
      shadowShader_ = renderer::Shader::Create("./src/shaders/shadow.vert", "./src/shaders/shadow.frag");
  }

  // 1a. Collect Entities
  std::vector<ecs::Entity> meshEntities;
  std::vector<ecs::Entity> lightEntities;
  for (auto const& entity : entities) {
      if (world.HasComponent<MeshComponent>(entity) && !world.HasComponent<SpriteComponent>(entity)) {
          meshEntities.push_back(entity);
      }
      if (world.HasComponent<LightComponent>(entity) && world.HasComponent<TransformComponent>(entity)) {
          lightEntities.push_back(entity);
      }
  }

  // 1b. Shadow Pass (Directional)
  Math::Mat4f lightSpaceMatrix = Math::Mat4f::Identity();
  ecs::Entity primaryLight = ecs::INVALID_ENTITY;
  for (auto const& le : lightEntities) {
      if (world.GetComponent<LightComponent>(le).Type == LightType::Directional && world.GetComponent<LightComponent>(le).CastShadows) {
          primaryLight = le;
          break;
      }
  }

  if (primaryLight != ecs::INVALID_ENTITY) {
      auto& lt = world.GetComponent<TransformComponent>(primaryLight);
      
      // Calculate Light projection (Ortho) and view
      Math::Vec4f dir4 = lt.rotation.ToMat4x4() * Math::Vec4f(0, 0, -1, 0);
      Math::Vec3f lightDir = { dir4.x, dir4.y, dir4.z };
      
      // Setup a simple ortho box for shadows (can be refined with CSM)
      float size = 20.0f;
      Math::Mat4f lightOrtho = Math::Mat4f::Orthographic(-size, size, -size, size, 0.1f, 100.0f);
      
      // View point: far away in light direction
      Math::Vec3f eye = (camera3D_ ? camera3D_->GetPosition() : Math::Vec3f{0}) - lightDir * 40.0f;
      Math::Vec3f center = (camera3D_ ? camera3D_->GetPosition() : Math::Vec3f{0});
      Math::Mat4f lightView = Math::Mat4f::LookAt(eye, center, {0, 1, 0});
      
      lightSpaceMatrix = lightOrtho * lightView;

      // Render to Shadow Map
      GLint oldViewport[4];
      glGetIntegerv(GL_VIEWPORT, oldViewport);

      shadowMap_->Bind();
      glViewport(0, 0, 2048, 2048);
      glClear(GL_DEPTH_BUFFER_BIT);
      
      shadowShader_->Bind();
      shadowShader_->SetMat4("u_LightSpaceMatrix", lightSpaceMatrix);

      for (auto const& me : meshEntities) {
          auto& transform = world.GetComponent<TransformComponent>(me);
          auto& meshComp = world.GetComponent<MeshComponent>(me);
          if (meshComp.MeshPtr) {
              Math::Mat4f model = Math::Mat4f::Translate(transform.position) *
                                  transform.rotation.ToMat4x4() *
                                  Math::Mat4f::Scale(transform.scale);
              shadowShader_->SetMat4("u_Model", model);
              meshComp.MeshPtr->Draw();
          }
      }
      shadowMap_->Unbind();
      glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);
  }

  // 2. 3D PBR Lighting Pass
  for (auto const &entity : meshEntities) {
      auto &transform = world.GetComponent<TransformComponent>(entity);
      auto &meshComp = world.GetComponent<MeshComponent>(entity);
      
      if (meshComp.MeshPtr && meshComp.MaterialPtr) {
        meshComp.MaterialPtr->Bind();
        auto shader = meshComp.MaterialPtr->GetShader();

        Math::Mat4f model = Math::Mat4f::Translate(transform.position) *
                            transform.rotation.ToMat4x4() *
                            Math::Mat4f::Scale(transform.scale);

        shader->SetMat4("u_Model", model);

        if (camera3D_) {
            shader->SetMat4("u_ViewProjection", camera3D_->GetViewProjectionMatrix());
            shader->SetVec3("u_CameraPos", camera3D_->GetPosition());
        }

        // Upload Lights
        int lightCount = (int)lightEntities.size();
        if (lightCount > 8) lightCount = 8;
        shader->SetInt("u_LightCount", lightCount);

        for (int i = 0; i < lightCount; ++i) {
            auto& lc = world.GetComponent<LightComponent>(lightEntities[i]);
            auto& lt = world.GetComponent<TransformComponent>(lightEntities[i]);

            std::string base = "u_Lights[" + std::to_string(i) + "].";
            shader->SetInt(base + "Type", (int)lc.Type);
            shader->SetVec3(base + "Position", lt.position);
            
            Math::Vec4f dir4 = lt.rotation.ToMat4x4() * Math::Vec4f(0, 0, -1, 0);
            Math::Vec3f direction = { dir4.x, dir4.y, dir4.z };
            shader->SetVec3(base + "Direction", direction);
            
            shader->SetVec3(base + "Color", lc.Color);
            shader->SetFloat(base + "Intensity", lc.Intensity);
            shader->SetFloat(base + "Range", lc.Range);
        }

        // Shadow Map Uniforms
        if (primaryLight != ecs::INVALID_ENTITY) {
            shader->SetMat4("u_LightSpaceMatrix", lightSpaceMatrix);
            // Bind shadow map to a known slot (e.g., slot 10)
            uint32_t shadowSlot = 10;
            glActiveTexture(GL_TEXTURE0 + shadowSlot);
            glBindTexture(GL_TEXTURE_2D, shadowMap_->GetDepthAttachmentRendererID());
            shader->SetInt("u_ShadowMap", (int)shadowSlot);
        }

        shader->SetVec3("u_AlbedoColor", meshComp.AlbedoColor);
        shader->SetFloat("u_Metallic", meshComp.Metallic);
        shader->SetFloat("u_Roughness", meshComp.Roughness);

        meshComp.MeshPtr->Draw();
      }
  }

  // 3. 2D Batch Pass
  if (camera2D_) {
      renderer::Renderer2D::BeginScene(*camera2D_);
      for (auto const &entity : entities) {
        if (world.HasComponent<SpriteComponent>(entity)) {
          auto &transform = world.GetComponent<TransformComponent>(entity);
          auto &spriteComp = world.GetComponent<SpriteComponent>(entity);
  
          Math::Vec2f uvTiling = spriteComp.tiling;
          Math::Vec2f uvOffset = {0.0f, 0.0f};
          if (spriteComp.framesX > 0 && spriteComp.framesY > 0) {
            float frameWidth = 1.0f / (float)spriteComp.framesX;
            float frameHeight = 1.0f / (float)spriteComp.framesY;
            uvTiling = {frameWidth, frameHeight};
            int column = spriteComp.currentFrame % spriteComp.framesX;
            int row = spriteComp.currentFrame / spriteComp.framesX;
            uvOffset = {(float)column * frameWidth, (float)(spriteComp.framesY - 1 - row) * frameHeight};
          }
          Math::Vec2f size = {transform.scale.x, transform.scale.y};
          if (spriteComp.texture)
            renderer::Renderer2D::DrawQuad(transform.position, size, spriteComp.texture, spriteComp.color, (int)entity.GetIndex(), spriteComp.FlipX, spriteComp.FlipY, uvTiling, uvOffset);
          else
            renderer::Renderer2D::DrawQuad(transform.position, size, spriteComp.color, (int)entity.GetIndex());
        }
      }
      renderer::Renderer2D::EndScene();
  }
}

} // namespace ecs
} // namespace ge
