#include "RenderSystem.h"
#include "../World.h"
#include "../components/TransformComponent.h"
#include "../components/SpriteComponent.h"
#include "../components/MeshComponent.h"
#include "../components/ModelComponent.h"
#include "../components/AnimatorComponent.h"
#include "../components/LightComponent.h"
#include <glad/glad.h>
#include "../../renderer/Renderer2D.h"
#include "../../renderer/RendererAPI.h"
#include "../../renderer/Shader.h"
#include "../../renderer/PerspectiveCamera.h"
#include "../../renderer/Framebuffer.h"
#include "../../renderer/Material.h"
#include "../../renderer/Mesh.h"
#include "../../renderer/Model.h"
#include "../../renderer/Texture.h"
#include "../../renderer/Cubemap.h"
#include "../components/SkyboxComponent.h"
#include <vector>

namespace ge {
namespace ecs {

  void RenderSystem::Render(World &world, float dt) {
    (void)dt;

    // 1. 2D Batch Pass (Moved to Top for Isolation)
    if (camera2D_) {
        renderer::Renderer2D::BeginScene(*camera2D_);
        for (auto e : entities) {
            if (world.HasComponent<SpriteComponent>(e)) {
                auto& tc = world.GetComponent<TransformComponent>(e);
                auto& sc = world.GetComponent<SpriteComponent>(e);

                Math::Vec2f uvT = sc.tiling;
                Math::Vec2f uvO = Math::Vec2f(0.0f, 0.0f);
                if (sc.framesX > 0 && sc.framesY > 0) {
                    float frameW = 1.0f / (float)sc.framesX;
                    float frameH = 1.0f / (float)sc.framesY;
                    uvT = Math::Vec2f(frameW, frameH);
                    int col = sc.currentFrame % sc.framesX;
                    int row = sc.currentFrame / sc.framesX;
                    uvO = Math::Vec2f((float)col * frameW, (float)(sc.framesY - 1 - row) * frameH);
                }
                Math::Vec2f quadSize = Math::Vec2f(tc.scale.x, tc.scale.y);

                if (sc.texture)
                    renderer::Renderer2D::DrawQuad(tc.position, quadSize, sc.texture, sc.color, (int)e.GetIndex(), sc.FlipX, sc.FlipY, uvT, uvO);
                else
                    renderer::Renderer2D::DrawQuad(tc.position, quadSize, sc.color, (int)e.GetIndex());
            }
        }
        renderer::Renderer2D::EndScene();
    }

    // --- IBL & Skybox Setup ---
    ecs::Entity skyboxEntity = ecs::INVALID_ENTITY;
    for (auto e : entities) {
        if (world.HasComponent<SkyboxComponent>(e)) {
            skyboxEntity = e;
            break;
        }
    }

    if (skyboxEntity != ecs::INVALID_ENTITY) {
        auto& skybox = world.GetComponent<SkyboxComponent>(skyboxEntity);
        if (skybox.SceneEnvironment && !skybox.SceneEnvironment->IsComputed) {
            SetupEnvironment(skybox);
        }
    }

    // 2. Setup Shadow System (Lazy Init)
    if (!shadowMap_) {
        renderer::FramebufferSpecification shadowSpec;
        shadowSpec.Width = 2048;
        shadowSpec.Height = 2048;
        shadowSpec.Attachments = { renderer::FramebufferTextureFormat::DEPTH24STENCIL8 };
        shadowMap_ = renderer::Framebuffer::Create(shadowSpec);
        shadowShader_ = renderer::Shader::Create("./src/shaders/shadow.vert", "./src/shaders/shadow.frag");
    }

    // 3. Collect 3D Entities
    std::vector<ecs::Entity> meshEntities;
    std::vector<ecs::Entity> modelEntities;
    std::vector<ecs::Entity> lightEntities;
    for (auto const& entity : entities) {
        if (world.HasComponent<MeshComponent>(entity) && !world.HasComponent<SpriteComponent>(entity)) {
            meshEntities.push_back(entity);
        }
        if (world.HasComponent<ModelComponent>(entity)) {
            modelEntities.push_back(entity);
        }
        if (world.HasComponent<LightComponent>(entity) && world.HasComponent<TransformComponent>(entity)) {
            lightEntities.push_back(entity);
        }
    }

    // 4. Shadow Pass (Directional)
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
        
        Math::Vec4f dir4 = lt.rotation.ToMat4x4() * Math::Vec4f(0, 0, -1, 0);
        Math::Vec3f lightDir = { dir4.x, dir4.y, dir4.z };
        
        float size = 20.0f;
        Math::Mat4f lightOrtho = Math::Mat4f::Orthographic(-size, size, -size, size, 0.1f, 100.0f);
        
        Math::Vec3f eye = (camera3D_ ? camera3D_->GetPosition() : Math::Vec3f{0}) - lightDir * 40.0f;
        Math::Vec3f center = (camera3D_ ? camera3D_->GetPosition() : Math::Vec3f{0});
        Math::Mat4f lightView = Math::Mat4f::LookAt(eye, center, {0, 1, 0});
        
        lightSpaceMatrix = lightOrtho * lightView;

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
                shadowShader_->SetBool("u_IsAnimated", false);
                meshComp.MeshPtr->Draw();
            }
        }

        for (auto const& me : modelEntities) {
            auto& transform = world.GetComponent<TransformComponent>(me);
            auto& modelComp = world.GetComponent<ModelComponent>(me);
            if (modelComp.ModelPtr) {
                Math::Mat4f model = Math::Mat4f::Translate(transform.position) *
                                    transform.rotation.ToMat4x4() *
                                    Math::Mat4f::Scale(transform.scale);
                shadowShader_->SetMat4("u_Model", model);
                
                bool isAnimated = false;
                if (world.HasComponent<AnimatorComponent>(me)) {
                    auto& animator = world.GetComponent<AnimatorComponent>(me);
                    if (animator.Is3D) {
                        isAnimated = true;
                        shadowShader_->SetMat4Array("u_BoneMatrices", animator.FinalBoneMatrices.data(), (uint32_t)animator.FinalBoneMatrices.size());
                    }
                }
                shadowShader_->SetBool("u_IsAnimated", isAnimated);
                
                for (auto& node : modelComp.ModelPtr->GetMeshes())
                    node.MeshPtr->Draw();
            }
        }
        shadowMap_->Unbind();
        glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);
    }

    // 5. 3D PBR Lighting Pass
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

            if (primaryLight != ecs::INVALID_ENTITY) {
                shader->SetMat4("u_LightSpaceMatrix", lightSpaceMatrix);
                uint32_t shadowSlot = 10;
                glActiveTexture(GL_TEXTURE0 + shadowSlot);
                glBindTexture(GL_TEXTURE_2D, shadowMap_->GetDepthAttachmentRendererID());
                shader->SetInt("u_ShadowMap", (int)shadowSlot);
            }

            shader->SetVec3("u_AlbedoColor", meshComp.AlbedoColor);
            shader->SetFloat("u_Metallic", meshComp.Metallic);
            shader->SetFloat("u_Roughness", meshComp.Roughness);
            shader->SetBool("u_IsAnimated", false);

            // IBL Bindings
            if (skyboxEntity != ecs::INVALID_ENTITY) {
                auto& skybox = world.GetComponent<SkyboxComponent>(skyboxEntity);
                if (skybox.SceneEnvironment && skybox.SceneEnvironment->IsComputed) {
                    skybox.SceneEnvironment->IrradianceMap->Bind(11);
                    skybox.SceneEnvironment->PrefilterMap->Bind(12);
                    // BrdfLUT binding (since it's a raw ID for now, I'll bind manually)
                    // Wait, I should have wrapped it in a Texture.
                    // For now, I'll use glBindTexture directly if needed, or better, keep slots consistent.
                    shader->SetInt("u_IrradianceMap", 11);
                    shader->SetInt("u_PrefilterMap", 12);
                    shader->SetBool("u_UseIBL", true);
                } else {
                    shader->SetBool("u_UseIBL", false);
                }
            } else {
                shader->SetBool("u_UseIBL", false);
            }

            meshComp.MeshPtr->Draw();
        }
    }

    // New Model Pass
    for (auto const &entity : modelEntities) {
        auto &transform = world.GetComponent<TransformComponent>(entity);
        auto &modelComp = world.GetComponent<ModelComponent>(entity);
        
        if (modelComp.ModelPtr && world.HasComponent<MeshComponent>(entity)) {
             auto &meshComp = world.GetComponent<MeshComponent>(entity);
             if (meshComp.MaterialPtr) {
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

                if (primaryLight != ecs::INVALID_ENTITY) {
                    shader->SetMat4("u_LightSpaceMatrix", lightSpaceMatrix);
                    uint32_t shadowSlot = 10;
                    glActiveTexture(GL_TEXTURE0 + shadowSlot);
                    glBindTexture(GL_TEXTURE_2D, shadowMap_->GetDepthAttachmentRendererID());
                    shader->SetInt("u_ShadowMap", (int)shadowSlot);
                }

                shader->SetVec3("u_AlbedoColor", modelComp.AlbedoColor);
                shader->SetFloat("u_Metallic", modelComp.Metallic);
                shader->SetFloat("u_Roughness", modelComp.Roughness);

                // IBL Bindings
                if (skyboxEntity != ecs::INVALID_ENTITY) {
                    auto& skybox = world.GetComponent<SkyboxComponent>(skyboxEntity);
                    if (skybox.SceneEnvironment && skybox.SceneEnvironment->IsComputed) {
                        skybox.SceneEnvironment->IrradianceMap->Bind(11);
                        skybox.SceneEnvironment->PrefilterMap->Bind(12);
                        shader->SetInt("u_IrradianceMap", 11);
                        shader->SetInt("u_PrefilterMap", 12);
                        shader->SetBool("u_UseIBL", true);
                    } else {
                        shader->SetBool("u_UseIBL", false);
                    }
                } else {
                    shader->SetBool("u_UseIBL", false);
                }

                bool isAnimated = false;
                if (world.HasComponent<AnimatorComponent>(entity)) {
                    auto& animator = world.GetComponent<AnimatorComponent>(entity);
                    if (animator.Is3D) {
                        isAnimated = true;
                        shader->SetMat4Array("u_BoneMatrices", animator.FinalBoneMatrices.data(), (uint32_t)animator.FinalBoneMatrices.size());
                    }
                }
                shader->SetBool("u_IsAnimated", isAnimated);

                for (auto& node : modelComp.ModelPtr->GetMeshes())
                    node.MeshPtr->Draw();
             }
        }
    }

    // 6. Skybox Pass
    if (skyboxEntity != ecs::INVALID_ENTITY) {
        auto& skybox = world.GetComponent<SkyboxComponent>(skyboxEntity);
        if (skybox.SceneEnvironment && skybox.SceneEnvironment->IsComputed && camera3D_) {
            glDepthFunc(GL_LEQUAL);
            static std::shared_ptr<renderer::Shader> skyShader = renderer::Shader::Create("./src/shaders/skybox.glsl");
            skyShader->Bind();
            
            // Remove translation from view matrix
            Math::Mat4f view = camera3D_->GetViewMatrix();
            view[0][3] = 0; view[1][3] = 0; view[2][3] = 0; // Column 3 (translation) in our Mat4 mapping
            // Wait, check Mat4 mapping. In my Mat4, translation is usually column 3 or row 3.
            // Simplified: just zero out the parts that don't belong to the 3x3
            Math::Mat4f view3x3 = Math::Mat4f::Identity();
            for(int i=0; i<3; i++) for(int j=0; j<3; j++) view3x3[i][j] = view[i][j];

            skyShader->SetMat4("u_View", view3x3);
            skyShader->SetMat4("u_Projection", camera3D_->GetProjectionMatrix());
            skybox.SceneEnvironment->EnvCubemap->Bind(0);
            skyShader->SetInt("u_Skybox", 0);

            RenderCube();
            glDepthFunc(GL_LESS);
        }
    }
  }

  // --- IBL Helper Methods ---

  void RenderSystem::RenderCube() {
      static uint32_t cubeVAO = 0, cubeVBO = 0;
      if (cubeVAO == 0) {
          float vertices[] = {
              -1,  1, -1, -1, -1, -1,  1, -1, -1,  1, -1, -1,  1,  1, -1, -1,  1, -1,
              -1, -1,  1, -1, -1, -1, -1,  1, -1, -1,  1, -1, -1,  1,  1, -1, -1,  1,
               1, -1, -1,  1, -1,  1,  1,  1,  1,  1,  1,  1,  1,  1, -1,  1, -1, -1,
              -1, -1,  1,  1, -1,  1,  1,  1,  1,  1,  1,  1, -1,  1,  1, -1, -1,  1,
              -1,  1, -1,  1,  1, -1,  1,  1,  1,  1,  1,  1, -1,  1,  1, -1,  1, -1,
              -1, -1, -1, -1, -1,  1,  1, -1, -1,  1, -1, -1, -1, -1,  1,  1, -1,  1
          };
          glCreateVertexArrays(1, &cubeVAO);
          glCreateBuffers(1, &cubeVBO);
          glNamedBufferStorage(cubeVBO, sizeof(vertices), vertices, 0);
          glVertexArrayVertexBuffer(cubeVAO, 0, cubeVBO, 0, 3 * sizeof(float));
          glEnableVertexArrayAttrib(cubeVAO, 0);
          glVertexArrayAttribFormat(cubeVAO, 0, 3, GL_FLOAT, GL_FALSE, 0);
          glVertexArrayAttribBinding(cubeVAO, 0, 0);
      }
      glBindVertexArray(cubeVAO);
      glDrawArrays(GL_TRIANGLES, 0, 36);
  }

  void RenderSystem::RenderQuad() {
      static uint32_t quadVAO = 0, quadVBO = 0;
      if (quadVAO == 0) {
          float vertices[] = {
              -1,  1, 0, 0, 1, -1, -1, 0, 0, 0,  1,  1, 0, 1, 1,  1, -1, 0, 1, 0
          };
          glCreateVertexArrays(1, &quadVAO);
          glCreateBuffers(1, &quadVBO);
          glNamedBufferStorage(quadVBO, sizeof(vertices), vertices, 0);
          glVertexArrayVertexBuffer(quadVAO, 0, quadVBO, 0, 5 * sizeof(float));
          glEnableVertexArrayAttrib(quadVAO, 0);
          glVertexArrayAttribFormat(quadVAO, 0, 3, GL_FLOAT, GL_FALSE, 0);
          glVertexArrayAttribBinding(quadVAO, 0, 0);
          glEnableVertexArrayAttrib(quadVAO, 1);
          glVertexArrayAttribFormat(quadVAO, 1, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
          glVertexArrayAttribBinding(quadVAO, 1, 0);
      }
      glBindVertexArray(quadVAO);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  }

  void RenderSystem::SetupEnvironment(SkyboxComponent& skybox) {
      if (!skybox.SceneEnvironment || skybox.SceneEnvironment->SourceHDRPath.empty()) return;
      auto& env = *skybox.SceneEnvironment;
      
      auto hdrTex = renderer::Texture::Create(env.SourceHDRPath, true);
      
      // 1. Equirect to Cubemap
      env.EnvCubemap = renderer::Cubemap::Create(512, true);
      static auto equirectShader = renderer::Shader::Create("./src/shaders/equirect_to_cubemap.glsl");
      
      Math::Mat4f captureProj = Math::Mat4f::Perspective(90.0f, 1.0f, 0.1f, 10.0f);
      Math::Mat4f captureViews[] = {
          Math::Mat4f::LookAt({0,0,0}, {1, 0, 0}, {0,-1, 0}),
          Math::Mat4f::LookAt({0,0,0}, {-1, 0, 0}, {0,-1, 0}),
          Math::Mat4f::LookAt({0,0,0}, {0, 1, 0}, {0, 0, 1}),
          Math::Mat4f::LookAt({0,0,0}, {0, -1, 0}, {0, 0, -1}),
          Math::Mat4f::LookAt({0,0,0}, {0, 0, 1}, {0,-1, 0}),
          Math::Mat4f::LookAt({0,0,0}, {0, 0, -1}, {0,-1, 0})
      };

      uint32_t captureFBO;
      glCreateFramebuffers(1, &captureFBO);
      glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

      glViewport(0, 0, 512, 512);
      equirectShader->Bind();
      equirectShader->SetMat4("u_Projection", captureProj);
      hdrTex->Bind(0);
      equirectShader->SetInt("u_EquirectangularMap", 0);

      for (int i = 0; i < 6; ++i) {
          equirectShader->SetMat4("u_View", captureViews[i]);
          glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, env.EnvCubemap->GetID(), 0);
          glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
          RenderCube();
      }

      // 2. Convolution
      env.IrradianceMap = renderer::Cubemap::Create(32, true);
      static auto irrShader = renderer::Shader::Create("./src/shaders/irradiance_convolution.glsl");
      irrShader->Bind();
      irrShader->SetMat4("u_Projection", captureProj);
      env.EnvCubemap->Bind(0);
      irrShader->SetInt("u_EnvironmentMap", 0);

      glViewport(0, 0, 32, 32);
      for (int i = 0; i < 6; ++i) {
          irrShader->SetMat4("u_View", captureViews[i]);
          glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, env.IrradianceMap->GetID(), 0);
          glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
          RenderCube();
      }

      // 3. Pre-filter
      env.PrefilterMap = renderer::Cubemap::Create(128, true);
      // Setup mipmaps for pre-filter map
      glGenerateTextureMipmap(env.PrefilterMap->GetID());

      static auto prefShader = renderer::Shader::Create("./src/shaders/prefilter.glsl");
      prefShader->Bind();
      prefShader->SetMat4("u_Projection", captureProj);
      env.EnvCubemap->Bind(0);
      prefShader->SetInt("u_EnvironmentMap", 0);

      unsigned int maxMipLevels = 5;
      for (unsigned int mip = 0; mip < maxMipLevels; ++mip) {
          unsigned int mipSize = (unsigned int)(128 * std::pow(0.5, mip));
          glViewport(0, 0, mipSize, mipSize);
          float roughness = (float)mip / (float)(maxMipLevels - 1);
          prefShader->SetFloat("u_Roughness", roughness);
          for (int i = 0; i < 6; ++i) {
              prefShader->SetMat4("u_View", captureViews[i]);
              glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, env.PrefilterMap->GetID(), mip);
              glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
              RenderCube();
          }
      }

      // 4. BRDF LUT
      // I'll skip BRDF LUT generation for now and use a default/placeholder or implement it if possible.
      // Actually, I'll implement it as it's needed for Split-Sum.
      uint32_t brdfLutID;
      glCreateTextures(GL_TEXTURE_2D, 1, &brdfLutID);
      glTextureStorage2D(brdfLutID, 1, GL_RG16F, 512, 512);
      glTextureParameteri(brdfLutID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTextureParameteri(brdfLutID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTextureParameteri(brdfLutID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTextureParameteri(brdfLutID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      glViewport(0, 0, 512, 512);
      static auto brdfShader = renderer::Shader::Create("./src/shaders/brdf.glsl");
      brdfShader->Bind();
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLutID, 0);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      RenderQuad();
      
      // Need a way to wrap brdfLutID into a Texture object.
      // env.BrdfLUT = ...

      glDeleteFramebuffers(1, &captureFBO);
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      
      env.IsComputed = true;
  }

} // namespace ecs
} // namespace ge
