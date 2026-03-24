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
  }

} // namespace ecs
} // namespace ge
