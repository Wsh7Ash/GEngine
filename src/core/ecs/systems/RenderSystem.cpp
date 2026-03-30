#include <glad/glad.h>
#include "RenderSystem.h"
#include <cstdint>
#include <string>
#include "../World.h"
#include "../components/TransformComponent.h"
#include "../components/SpriteComponent.h"
#include "../components/MeshComponent.h"
#include "../components/DecalComponent.h"
#include "../components/ModelComponent.h"
#include "../components/AnimatorComponent.h"
#include "../components/LightComponent.h"
#include "../components/TagComponent.h"
#include "../components/RelationshipComponent.h"
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
#include "../../renderer/PostProcessingPass.h"
#include "../../math/BoundingVolumes.h"

// Decal shaders
#include "../../../shaders/decal.vert.glsl"
#include "../../../shaders/decal.frag.glsl"
#include "../components/SkyboxComponent.h"
#include <vector>
#include <random>
#include <chrono>

namespace ge {
namespace ecs {

struct ScopedProfileTimer {
    std::chrono::time_point<std::chrono::high_resolution_clock> Start;
    float* Result;
    ScopedProfileTimer(float* result) : Result(result) {
        Start = std::chrono::high_resolution_clock::now();
    }
    ~ScopedProfileTimer() {
        auto end = std::chrono::high_resolution_clock::now();
        *Result = std::chrono::duration<float, std::milli>(end - Start).count();
    }
};


  void RenderSystem::Render(World &world, float dt) {
    (void)dt;

    // Fetch Viewport for jitter and post-processing setup
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    float width = viewport[2] > 0 ? (float)viewport[2] : 1920.0f;
    float height = viewport[3] > 0 ? (float)viewport[3] : 1080.0f;

    // 1. TAA Jitter and Matrix Update
    if (camera3D_) {
        frameIndex_++;
        // Halton(2,3) sequence for jittering
        auto halton = [](int i, int b) {
            float f = 1.0, r = 0.0;
            while (i > 0) {
                f /= b;
                r += f * (i % b);
                i /= b;
            }
            return r;
        };
        
        float jx = halton((frameIndex_ % 16) + 1, 2) - 0.5f;
        float jy = halton((frameIndex_ % 16) + 1, 3) - 0.5f;
        
        jitter_ = { jx / width, jy / height }; 
        
        // Apply jitter to projection matrix
        // (Simplified for now: we will pass jitter as a uniform or adjust proj matrix)
        // Usually, in OpenGL proj[2][0] += jitter.x; proj[2][1] += jitter.y;
    }

    // Collect entities from ECS using Query
    std::vector<::ge::ecs::Entity> allEntities;
    for (auto e : world.Query<::ge::ecs::TagComponent>()) {
        allEntities.push_back(e);
    }
    for (auto e : allEntities) {
        auto& tag = world.GetTag(e);
        if (tag.tag == "Camera") {
            // Found a camera entity, potentially set camera3D_ or similar
            // This block was incomplete in the instruction, so leaving it as is.
        }
    }
    // 2D Batch Pass moved to the bottom of Render function

    // --- Post-Processing Stack Initialization ---
    if (!postProcessingStack_) {
        postProcessingStack_ = std::make_shared<renderer::PostProcessingStack>();
        
        // Initialize intermediate framebuffers
        renderer::FramebufferSpecification spec;
        spec.Width = viewport[2] > 0 ? viewport[2] : 1920; 
        spec.Height = viewport[3] > 0 ? viewport[3] : 1080;
        spec.Attachments = { renderer::FramebufferTextureFormat::RGBA8, renderer::FramebufferTextureFormat::Depth };
        
        intermediateA_ = renderer::Framebuffer::Create(spec);
        intermediateB_ = renderer::Framebuffer::Create(spec);

         // SSAO G-Buffer - Modified for decals
         renderer::FramebufferSpecification gSpec;
         gSpec.Width = spec.Width;
         gSpec.Height = spec.Height;
         gSpec.Attachments = {
             renderer::FramebufferTextureFormat::RGBA16F, // Position
             renderer::FramebufferTextureFormat::RGBA16F, // Albedo (RGB) + Metallic (A)
             renderer::FramebufferTextureFormat::RGBA16F, // Normal (RGB) + Roughness (A)
             renderer::FramebufferTextureFormat::RG16F,   // Velocity
             renderer::FramebufferTextureFormat::RGBA16F, // Subsurface (RGB) + Thickness (A)
             renderer::FramebufferTextureFormat::Depth
         };
        gBuffer_ = renderer::Framebuffer::Create(gSpec);
        
        // TAA FBOs
        renderer::FramebufferSpecification taaSpec;
        taaSpec.Width = spec.Width;
        taaSpec.Height = spec.Height;
        taaSpec.Attachments = { renderer::FramebufferTextureFormat::RGBA16F };
        prevFrameFBO_ = renderer::Framebuffer::Create(taaSpec);
        resolveFBO_ = renderer::Framebuffer::Create(taaSpec);

         // SSAO FBOs
        renderer::FramebufferSpecification ssaoSpec;
        ssaoSpec.Width = spec.Width;
        ssaoSpec.Height = spec.Height;
        ssaoSpec.Attachments = { renderer::FramebufferTextureFormat::RED8 };
        ssaoFBO_ = renderer::Framebuffer::Create(ssaoSpec);
        ssaoBlurFBO_ = renderer::Framebuffer::Create(ssaoSpec);

        // SSGI FBO
        renderer::FramebufferSpecification ssgiSpec;
        ssgiSpec.Width = spec.Width;
        ssgiSpec.Height = spec.Height;
        ssgiSpec.Attachments = { renderer::FramebufferTextureFormat::RGBA16F }; // HDR for indirect lighting
        ssgiFBO_ = renderer::Framebuffer::Create(ssgiSpec);

        // SSR FBO
        renderer::FramebufferSpecification ssrSpec;
        ssrSpec.Width = spec.Width;
        ssrSpec.Height = spec.Height;
        ssrSpec.Attachments = { renderer::FramebufferTextureFormat::RGBA16F }; // HDR for reflection
        ssrFBO_ = renderer::Framebuffer::Create(ssrSpec);

        // Sample Kernel
        // Sample Kernel
        std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
        std::default_random_engine generator;
        for (unsigned int i = 0; i < 64; ++i) {
            Math::Vec3f sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator));
            sample = sample.Normalized();
            sample *= randomFloats(generator);
            float scale = (float)i / 64.0f;
            scale = 0.1f + scale * scale * (1.0f - 0.1f); // Lerp
            sample *= scale;
            ssaoKernel_.push_back(sample);
        }

        // Noise Texture
        std::vector<Math::Vec3f> ssaoNoise;
        for (unsigned int i = 0; i < 16; i++) {
            Math::Vec3f noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f);
            ssaoNoise.push_back(noise);
        }
        glGenTextures(1, &ssaoNoiseTexture_);
        glBindTexture(GL_TEXTURE_2D, ssaoNoiseTexture_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // Load Shaders
        gBufferShader_ = renderer::Shader::Create("./src/shaders/gbuffer.vert.glsl", "./src/shaders/gbuffer.frag.glsl");
        ssaoShader_    = renderer::Shader::Create("./src/shaders/postprocess.vert.glsl", "./src/shaders/ssao.glsl");
        ssaoBlurShader_ = renderer::Shader::Create("./src/shaders/postprocess.vert.glsl", "./src/shaders/ssao_blur.glsl");
        
        // TAA
        taaShader_ = renderer::Shader::Create("./src/shaders/postprocess.vert.glsl", "./src/shaders/taa.glsl");
        
         // Volumetric
          volumetricShader_ = renderer::Shader::Create("./src/shaders/postprocess.vert.glsl", "./src/shaders/volumetric_lighting.glsl");
          
          // Decal
          decalShader_ = renderer::Shader::Create("./src/shaders/decal.vert.glsl", "./src/shaders/decal.frag.glsl");
          
           // SSGI
           ssgiShader_ = renderer::Shader::Create("./src/shaders/postprocess.vert.glsl", "./src/shaders/ssgi.glsl");
           
           // SSR
           ssrShader_ = renderer::Shader::Create("./src/shaders/postprocess.vert.glsl", "./src/shaders/ssr.glsl");

           // Plasma
           plasmaShader_ = renderer::Shader::Create("./src/shaders/postprocess.vert.glsl", "./src/shaders/plasma.glsl");
           
           // SSS (Subsurface Scattering)
           ssssShader_ = renderer::Shader::Create("./src/shaders/postprocess.vert.glsl", "./src/shaders/ssss.glsl");
           
           // Refraction
           refractionShader_ = renderer::Shader::Create("./src/shaders/postprocess.vert.glsl", "./src/shaders/refraction.glsl");
           
          renderer::FramebufferSpecification volSpec;
         volSpec.Width = spec.Width / 2; // Half-res for performance
         volSpec.Height = spec.Height / 2;
         volSpec.Attachments = { renderer::FramebufferTextureFormat::RGBA16F }; // HDR
         volumetricFBO_ = renderer::Framebuffer::Create(volSpec);

         // SSS FBO
         renderer::FramebufferSpecification ssssSpec;
         ssssSpec.Width = spec.Width;
         ssssSpec.Height = spec.Height;
         ssssSpec.Attachments = { renderer::FramebufferTextureFormat::RGBA16F }; // HDR
         ssssFBO_ = renderer::Framebuffer::Create(ssssSpec);

         // Refraction FBO
         renderer::FramebufferSpecification refractSpec;
         refractSpec.Width = spec.Width;
         refractSpec.Height = spec.Height;
         refractSpec.Attachments = { renderer::FramebufferTextureFormat::RGBA16F }; // HDR
         refractionFBO_ = renderer::Framebuffer::Create(refractSpec);
    }

    // --- IBL & Skybox Setup ---
    ecs::Entity skyboxEntity = ecs::INVALID_ENTITY;
    for (auto e : allEntities) {
        if (world.HasSkybox(e)) {
            skyboxEntity = e;
            break;
        }
    }

    if (skyboxEntity != ecs::INVALID_ENTITY) {
        auto& skybox = world.GetSkybox(skyboxEntity);
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

     // --- Collect 3D Entities ---
     std::vector<::ge::ecs::Entity> meshEntities;
     std::vector<::ge::ecs::Entity> modelEntities;
     std::vector<::ge::ecs::Entity> lightEntities;
     std::vector<::ge::ecs::Entity> decalEntities;
     for (auto const& entity : allEntities) {
         if (world.HasMesh(entity) && !world.HasSprite(entity)) {
             meshEntities.push_back(entity);
         }
         if (world.HasModel(entity)) {
             modelEntities.push_back(entity);
         }
         if (world.HasLight(entity)) {
             lightEntities.push_back(entity);
         }
          if (world.HasDecal(entity)) {
              decalEntities.push_back(entity);
          }
      }

    // 3.5 Forward+ Cluster Setup
    if (settings_.EnableForwardPlus) {
        BuildForwardPlusClusters();
        AssignLightsToClusters(lightEntities, world);
    }

    // 4. Shadow Pass (Directional)
    Math::Mat4f lightSpaceMatrix = Math::Mat4f::Identity();
    ecs::Entity primaryLight = ecs::INVALID_ENTITY;
    for (auto const& le : lightEntities) {
        auto& lc = world.GetLight(le);
        if (lc.Type == ge::ecs::LightType::Directional && lc.CastShadows) {
            primaryLight = le;
            break;
        }
    }

    Math::Frustum frustum;
    if (camera3D_) {
        frustum.FromMatrix(camera3D_->GetViewProjectionMatrix());
    }

    if (primaryLight != ecs::INVALID_ENTITY) {
        auto& lt = world.GetTransform(primaryLight);
        
        Math::Vec4f dir4 = lt.rotation.ToMat4x4() * Math::Vec4f(0, 0, -1, 0);
        Math::Vec3f lightDir = { dir4.x, dir4.y, dir4.z };
        
        float size = 20.0f;
        Math::Mat4f lightOrtho = Math::Mat4f::Orthographic(-size, size, -size, size, 0.1f, 100.0f);
        
        Math::Vec3f eye = (camera3D_ ? camera3D_->GetPosition() : Math::Vec3f{0}) - lightDir * 40.0f;
        Math::Vec3f center = (camera3D_ ? camera3D_->GetPosition() : Math::Vec3f{0});
        Math::Mat4f lightView = Math::Mat4f::LookAt(eye, center, {0, 1, 0});
        
        Math::Mat4f res = lightOrtho * lightView;
        lightSpaceMatrix = res;

        GLint oldViewport[4];
        glGetIntegerv(GL_VIEWPORT, oldViewport);

        ScopedProfileTimer shadowTimer(&renderer::Renderer2D::GetStats().PassShadows);
        shadowMap_->Bind();
        glViewport(0, 0, 2048, 2048);
        glClear(GL_DEPTH_BUFFER_BIT);
        
        shadowShader_->SetMat4("u_LightSpaceMatrix", lightSpaceMatrix);

        for (auto const& me : meshEntities) {
            auto& transform = world.GetTransform(me);
            auto& meshComp = world.GetMesh(me);
            
            if (!meshComp.IsVisible) continue;

            if (meshComp.MeshPtr) {
                Math::Mat4f model = Math::Mat4f::Translate(transform.position) *
                                    transform.rotation.ToMat4x4() *
                                    Math::Mat4f::Scale(transform.scale);
                
                // Culling Test
                Math::AABB worldAABB = meshComp.MeshPtr->GetAABB().Transform(model);
                if (camera3D_ && !frustum.Intersect(worldAABB)) continue;

                shadowShader_->SetMat4("u_Model", model);
                shadowShader_->SetBool("u_IsAnimated", false);
                meshComp.MeshPtr->Draw();
            }
        }

        for (auto const& me : modelEntities) {
            auto& transform = world.GetTransform(me);
            auto& modelComp = world.GetModel(me);
            if (modelComp.ModelPtr) {
                Math::Mat4f model = Math::Mat4f::Translate(transform.position) *
                                    transform.rotation.ToMat4x4() *
                                    Math::Mat4f::Scale(transform.scale);
                
                // Culling Test
                Math::AABB worldAABB = modelComp.ModelPtr->GetAABB().Transform(model);
                if (camera3D_ && !frustum.Intersect(worldAABB)) continue;

                shadowShader_->SetMat4("u_Model", model);
                
                bool isAnimated = false;
                static_assert(sizeof(::ge::ecs::AnimatorComponent) > 0, "AnimatorComponent must be complete");
                if (world.HasAnimator(me)) {
                    auto& animator = world.GetAnimator(me);
                    if (animator.Is3D) {
                        isAnimated = true;
                        const char* boneUniformName = "u_BoneMatrices";
                        const Math::Mat4f* boneData = animator.FinalBoneMatrices.data();
                        uint32_t boneCount = static_cast<uint32_t>(animator.FinalBoneMatrices.size());
                        ::ge::renderer::Shader* shaderPtr = shadowShader_.get();
                        shaderPtr->SetMat4Array(boneUniformName, boneData, boneCount);
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

    // --- Begin Post-Processed 3D Pipeline ---
    // Render the 3D scene (PBR, Skybox) into an HDR intermediate buffer
    if (intermediateA_) {
        intermediateA_->Bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear HDR buffer
    }

    // 5. SSAO Pass
    if (settings_.EnableSSAO) {
        ExecuteSSAOPass(world);
    }

    // Re-bind Intermediate A for 3D PBR passes (SSAO pass unbinds it usually)
    if (intermediateA_) {
        intermediateA_->Bind();
    }

    // 6. 3D PBR Lighting Pass
    {
        ScopedProfileTimer lightingTimer(&renderer::Renderer2D::GetStats().PassLighting);
        static_assert(sizeof(::ge::ecs::TransformComponent) > 0, "TransformComponent must be complete");
        static_assert(sizeof(::ge::ecs::MeshComponent) > 0, "MeshComponent must be complete");
        for (auto const& entity : meshEntities) {
        auto& transform = world.GetTransform(entity);
        auto& meshComp = world.GetMesh(entity);

        if (!meshComp.MeshPtr) continue;
        if (!meshComp.IsVisible) continue;

        if (meshComp.MeshPtr && meshComp.MaterialPtr) {
            Math::Mat4f model = Math::Mat4f::Translate(transform.position) *
                                transform.rotation.ToMat4x4() *
                                Math::Mat4f::Scale(transform.scale);
            
            // Culling Test
            Math::AABB worldAABB = meshComp.MeshPtr->GetAABB().Transform(model);
            if (camera3D_ && !frustum.Intersect(worldAABB)) continue;

            // LOD Selection
            std::shared_ptr<renderer::Mesh> activeMesh = meshComp.MeshPtr;
            if (camera3D_ && !meshComp.LODLevels.empty()) {
                float dist = (transform.position - camera3D_->GetPosition()).Length();
                for (const auto& lod : meshComp.LODLevels) {
                    if (dist >= lod.DistanceThreshold)
                        activeMesh = lod.MeshPtr;
                }
            }

             meshComp.MaterialPtr->Bind();
             auto shader = meshComp.MaterialPtr->GetShader();
 
             shader->SetMat4("u_Model", model);
 
             if (camera3D_) {
                 shader->SetMat4("u_ViewProjection", camera3D_->GetViewProjectionMatrix());
                 shader->SetVec3("u_CameraPos", camera3D_->GetPosition());
             }
 
             int lightCount = (int)lightEntities.size();
             if (lightCount > 8) lightCount = 8;
             shader->SetInt("u_LightCount", lightCount);
 
             for (int i = 0; i < lightCount; ++i) {
                 auto& lc = world.GetLight(lightEntities[i]);
                 auto& lt = world.GetTransform(lightEntities[i]);
 
                 std::string base = "u_Lights[" + std::to_string(i) + "].";
                 shader->SetInt(base + "Type", (int)lc.Type);
                 shader->SetVec3(base + "Position", lt.position);
                 
                 Math::Vec4f dir4 = lt.rotation.ToMat4x4() * Math::Vec4f(0, 0, -1, 0);
                 Math::Vec3f direction = { dir4.x, dir4.y, dir4.z };
                 shader->SetVec3(base + "Direction", direction);
                 
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
 
             // Bind SSAO texture (Slot 5)
             if (settings_.EnableSSAO) {
                 glActiveTexture(GL_TEXTURE5);
                 glBindTexture(GL_TEXTURE_2D, ssaoBlurFBO_->GetColorAttachmentRendererID(0));
                 shader->SetInt("u_SSAO", 5);
             } else {
                 shader->SetInt("u_SSAO", -1); // Or handle in shader
             }
             
             // Bind G-Buffer textures for decal support
             glActiveTexture(GL_TEXTURE6);
             glBindTexture(GL_TEXTURE_2D, gBuffer_->GetColorAttachmentRendererID(0)); // Position
             shader->SetInt("u_gPosition", 6);

             glActiveTexture(GL_TEXTURE7);
             glBindTexture(GL_TEXTURE_2D, gBuffer_->GetColorAttachmentRendererID(1)); // Albedo
             shader->SetInt("u_gAlbedo", 7);

             glActiveTexture(GL_TEXTURE8);
             glBindTexture(GL_TEXTURE_2D, gBuffer_->GetColorAttachmentRendererID(2)); // Normal
             shader->SetInt("u_gNormal", 8);

             glActiveTexture(GL_TEXTURE9);
             glBindTexture(GL_TEXTURE_2D, gBuffer_->GetColorAttachmentRendererID(3)); // Velocity
             shader->SetInt("u_gVelocity", 9);

             // Bind SSGI texture (Slot 13)
             if (settings_.EnableSSGI && ssgiFBO_) {
                 glActiveTexture(GL_TEXTURE13);
                 glBindTexture(GL_TEXTURE_2D, ssgiFBO_->GetColorAttachmentRendererID(0));
                 shader->SetInt("u_SSGI", 13);
                 shader->SetFloat("u_SSGIIntensity", settings_.SSGIIntensity);
             } else {
                 shader->SetInt("u_SSGI", -1);
                 shader->SetFloat("u_SSGIIntensity", 0.0);
             }

              // Bind SSR texture (Slot 14)
              if (settings_.EnableSSR && ssrFBO_) {
                  glActiveTexture(GL_TEXTURE14);
                  glBindTexture(GL_TEXTURE_2D, ssrFBO_->GetColorAttachmentRendererID(0));
                  shader->SetInt("u_SSR", 14);
                  shader->SetFloat("u_SSRIntensity", settings_.SSRIntensity);
              } else {
                  shader->SetInt("u_SSR", -1);
                  shader->SetFloat("u_SSRIntensity", 0.0);
              }

              // Forward+ Clustered Lighting
              shader->SetBool("u_UseForwardPlus", settings_.EnableForwardPlus);
              if (settings_.EnableForwardPlus && lightBuffer_ != 0) {
                  shader->SetInt("u_ClusterCountX", clusterCountX_);
                  shader->SetInt("u_ClusterCountY", clusterCountY_);
                  shader->SetInt("u_ClusterCountZ", clusterCountZ_);
                  shader->SetFloat("u_NearPlane", camera3D_->GetNearPlane());
                  shader->SetFloat("u_FarPlane", camera3D_->GetFarPlane());

                  // Bind SSBOs
                  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, lightBuffer_);
                  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, clusterLightIndicesBuffer_);
                  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, clusterLightCountsBuffer_);
              }
              
              // Indicate we're using G-Buffer materials
             shader->SetBool("u_UseGBufferMaterials", true);
 
             // Set default material values (will be overridden by G-Buffer when used)
             shader->SetVec3("u_AlbedoColor", meshComp.AlbedoColor);
             shader->SetFloat("u_Metallic", meshComp.Metallic);
             shader->SetFloat("u_Roughness", meshComp.Roughness);
             shader->SetBool("u_IsAnimated", false);

            // IBL Bindings
            if (skyboxEntity != ecs::INVALID_ENTITY) {
                auto& skybox = world.GetSkybox(skyboxEntity);
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

            activeMesh->Draw();
        }
    }

    // New Model Pass
    for (auto const &entity : modelEntities) {
        auto &transform = world.GetTransform(entity);
        auto &modelComp = world.GetModel(entity);
        
        if (modelComp.ModelPtr && world.HasMesh(entity)) {
             auto &meshComp = world.GetMesh(entity);
             if (meshComp.MaterialPtr) {
                meshComp.MaterialPtr->Bind();
                auto shader = meshComp.MaterialPtr->GetShader();

                Math::Mat4f model = Math::Mat4f::Translate(transform.position) *
                                    transform.rotation.ToMat4x4() *
                                    Math::Mat4f::Scale(transform.scale);

                // Culling Test
                Math::AABB worldAABB = modelComp.ModelPtr->GetAABB().Transform(model);
                if (camera3D_ && !frustum.Intersect(worldAABB)) continue;

                 shader->SetMat4("u_Model", model);
 
                 if (camera3D_) {
                     shader->SetMat4("u_ViewProjection", camera3D_->GetViewProjectionMatrix());
                     shader->SetVec3("u_CameraPos", camera3D_->GetPosition());
                 }
 
                 int lightCount = (int)lightEntities.size();
                 if (lightCount > 8) lightCount = 8;
                 shader->SetInt("u_LightCount", lightCount);
 
                 for (int i = 0; i < lightCount; ++i) {
                     auto& lc = world.GetLight(lightEntities[i]);
                     auto& lt = world.GetTransform(lightEntities[i]);
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
 
                 // Bind G-Buffer textures for decal support
                 glActiveTexture(GL_TEXTURE6);
                 glBindTexture(GL_TEXTURE_2D, gBuffer_->GetColorAttachmentRendererID(0)); // Position
                 shader->SetInt("u_gPosition", 6);

                 glActiveTexture(GL_TEXTURE7);
                 glBindTexture(GL_TEXTURE_2D, gBuffer_->GetColorAttachmentRendererID(1)); // Albedo
                 shader->SetInt("u_gAlbedo", 7);

                 glActiveTexture(GL_TEXTURE8);
                 glBindTexture(GL_TEXTURE_2D, gBuffer_->GetColorAttachmentRendererID(2)); // Normal
                 shader->SetInt("u_gNormal", 8);

                 glActiveTexture(GL_TEXTURE9);
                 glBindTexture(GL_TEXTURE_2D, gBuffer_->GetColorAttachmentRendererID(3)); // Velocity
                 shader->SetInt("u_gVelocity", 9);

                 // Bind SSGI texture (Slot 13)
                 if (settings_.EnableSSGI && ssgiFBO_) {
                     glActiveTexture(GL_TEXTURE13);
                     glBindTexture(GL_TEXTURE_2D, ssgiFBO_->GetColorAttachmentRendererID(0));
                     shader->SetInt("u_SSGI", 13);
                     shader->SetFloat("u_SSGIIntensity", settings_.SSGIIntensity);
                 } else {
                     shader->SetInt("u_SSGI", -1);
                     shader->SetFloat("u_SSGIIntensity", 0.0);
                 }

                 // Bind SSR texture (Slot 14)
                  if (settings_.EnableSSR && ssrFBO_) {
                      glActiveTexture(GL_TEXTURE14);
                      glBindTexture(GL_TEXTURE_2D, ssrFBO_->GetColorAttachmentRendererID(0));
                      shader->SetInt("u_SSR", 14);
                      shader->SetFloat("u_SSRIntensity", settings_.SSRIntensity);
                  } else {
                      shader->SetInt("u_SSR", -1);
                      shader->SetFloat("u_SSRIntensity", 0.0);
                  }

                  // Forward+ Clustered Lighting
                  shader->SetBool("u_UseForwardPlus", settings_.EnableForwardPlus);
                  if (settings_.EnableForwardPlus && lightBuffer_ != 0) {
                      shader->SetInt("u_ClusterCountX", clusterCountX_);
                      shader->SetInt("u_ClusterCountY", clusterCountY_);
                      shader->SetInt("u_ClusterCountZ", clusterCountZ_);
                      shader->SetFloat("u_NearPlane", camera3D_->GetNearPlane());
                      shader->SetFloat("u_FarPlane", camera3D_->GetFarPlane());

                      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, lightBuffer_);
                      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, clusterLightIndicesBuffer_);
                      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, clusterLightCountsBuffer_);
                  }
                  
                  // In the decal implementation, we read material properties from the G-Buffer
                 // So we don't need to set these as uniforms - they'll be sampled in the shader
                 // However, we still need to indicate that we're using the G-Buffer for materials
                 shader->SetBool("u_UseGBufferMaterials", true);
 
                 // IBL Bindings
                if (skyboxEntity != ecs::INVALID_ENTITY) {
                    auto& skybox = world.GetSkybox(skyboxEntity);
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
                if (world.HasAnimator(entity)) {
                    auto& animator = world.GetAnimator(entity);
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
    } // Close lightingTimer scope

    // 6. Skybox Pass
    if (skyboxEntity != ecs::INVALID_ENTITY) {
        auto& skybox = world.GetSkybox(skyboxEntity);
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
    
    if (intermediateA_) {
        intermediateA_->Unbind(); // End 3D HDR scene pass
    }

    // Post-Process Initialization moved to the top of Render function.
    
    // 8. Resolve and Post-Processing ---
    // Volumetric Pass
    if (settings_.EnableVolumetric) {
        ExecuteVolumetricPass(world);
    }

    // SSS (Subsurface Scattering) Pass
    if (settings_.EnableSSSS) {
        ExecuteSSSSPass(world);
    }

    // Refraction Pass
    if (settings_.EnableRefraction) {
        ExecuteRefractionPass(world);
    }

    // Plasma Pass
    if (settings_.EnablePlasma) {
        ExecutePlasmaPass(world);
    }
    
    if (camera3D_ && postProcessingStack_) {
        ScopedProfileTimer ppTimer(&renderer::Renderer2D::GetStats().PassPostProcess);
        // TAA and Final Blit
        // 1. Bind TAA FBO
        resolveFBO_->Bind();
        glClear(GL_COLOR_BUFFER_BIT);
        
        taaShader_->Bind();
        
        // Bind HDR Color
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, intermediateA_->GetColorAttachmentRendererID(0));
        
        // Bind History
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, prevFrameFBO_->GetColorAttachmentRendererID(0));
        
        // Bind Velocity
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gBuffer_->GetColorAttachmentRendererID(2));
        
        taaShader_->SetInt("u_CurrentColor", 0);
        taaShader_->SetInt("u_HistoryColor", 1);
        taaShader_->SetInt("u_VelocityBlock", 2);
        
        if (settings_.EnableVolumetric) {
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, volumetricFBO_->GetColorAttachmentRendererID(0));
            taaShader_->SetInt("u_Volumetric", 3);
        } else {
            taaShader_->SetInt("u_Volumetric", -1);
        }
        
        RenderQuad();
        resolveFBO_->Unbind();
        
        // 2. Output to Screen
        // We draw the TAA resolved image to default framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, (GLsizei)width, (GLsizei)height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear default framebuffer for UI/2D
        
        static std::shared_ptr<renderer::Shader> blitShader = renderer::Shader::Create("./src/shaders/postprocess.vert.glsl", "./src/shaders/postprocess.frag.glsl");
        blitShader->Bind();
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, resolveFBO_->GetColorAttachmentRendererID(0));
        blitShader->SetInt("tex", 0);
        
        // Disable Depth Test to allow drawing over entire screen
        glDisable(GL_DEPTH_TEST);
        RenderQuad();
        glEnable(GL_DEPTH_TEST);
        
        // 3. Update History Buffer
        // Simplest: Blit resolveFBO to prevFrameFBO
        prevFrameFBO_->Bind();
        blitShader->Bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, resolveFBO_->GetColorAttachmentRendererID(0));
        blitShader->SetInt("tex", 0);
        RenderQuad();
        prevFrameFBO_->Unbind();
        
    } else if (intermediateA_) {
        // Simple blit if no TAA/Camera3D
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, (GLsizei)width, (GLsizei)height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        static std::shared_ptr<renderer::Shader> blitShader = renderer::Shader::Create("./src/shaders/postprocess.vert.glsl", "./src/shaders/postprocess.frag.glsl");
        blitShader->Bind();
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, intermediateA_->GetColorAttachmentRendererID(0));
        blitShader->SetInt("tex", 0);
        glDisable(GL_DEPTH_TEST);
        RenderQuad();
        glEnable(GL_DEPTH_TEST);
    }
    
    // --- 9. 2D Pass ---
    
    // 2D Batch Pass (Always render last, so it's on top of 3D and UI)
    if (camera2D_) {
        renderer::Renderer2D::BeginScene(*camera2D_);
        for (auto e : entities) {
            if (world.HasSprite(e)) {
                auto& tc = world.GetTransform(e);
                auto& sc = world.GetSprite(e);

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
  }

  void RenderSystem::ExecuteVolumetricPass(World& world) {
    if (!camera3D_ || !volumetricFBO_ || !shadowMap_) return;
    ScopedProfileTimer volTimer(&renderer::Renderer2D::GetStats().PassVolumetric);

    volumetricFBO_->Bind();
    glClear(GL_COLOR_BUFFER_BIT);
    
    volumetricShader_->Bind();
    
    // Bind Depth and Shadow Maps
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gBuffer_->GetDepthAttachmentRendererID());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, shadowMap_->GetDepthAttachmentRendererID());
    
    volumetricShader_->SetInt("u_DepthMap", 0);
    volumetricShader_->SetInt("u_ShadowMap", 1);
    
    volumetricShader_->SetMat4("u_InverseViewProj", camera3D_->GetViewProjectionMatrix().Inverse());
    // Find primary light
    auto lightEntities = world.Query<ge::ecs::LightComponent, ge::ecs::TransformComponent>();
    ge::ecs::Entity primaryLight = ge::ecs::INVALID_ENTITY;
    for (auto e : lightEntities) {
        if (world.GetLight(e).Type == ge::ecs::LightType::Directional) {
            primaryLight = e;
            break;
        }
    }
    
    if (primaryLight != ge::ecs::INVALID_ENTITY) {
        auto& lt = world.GetTransform(primaryLight);
        auto& lc = world.GetLight(primaryLight);
        volumetricShader_->SetVec3("u_LightPos", lt.position);
        
        // Use the same light space matrix as shadows
        Math::Mat4f lightProjection = Math::Mat4f::Orthographic(-20.0f, 20.0f, -20.0f, 20.0f, 0.1f, 100.0f);
        Math::Mat4f lightView = lt.rotation.ToMat4x4().Inverse() * Math::Mat4f::Translate(-lt.position);
        Math::Mat4f res = lightProjection * lightView;
        volumetricShader_->SetMat4("u_LightSpaceMatrix", res);

        // Pass Volumetric Settings
        volumetricShader_->SetFloat("u_Scattering", settings_.VolumetricScattering);
        volumetricShader_->SetFloat("u_Intensity", settings_.VolumetricIntensity);
        volumetricShader_->SetInt("u_Samples", settings_.VolumetricSamples);
    }

    volumetricShader_->SetVec3("u_CameraPos", camera3D_->GetPosition());
    
    RenderQuad();
    volumetricFBO_->Unbind();
  }

  void RenderSystem::ExecutePlasmaPass(World& world) {
      (void)world;
      if (!plasmaShader_ || !intermediateA_) return;

      intermediateB_->Bind();
      glClear(GL_COLOR_BUFFER_BIT);

      plasmaShader_->Bind();

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, intermediateA_->GetColorAttachmentRendererID(0));
      plasmaShader_->SetInt("u_Texture", 0);
      plasmaShader_->SetFloat("u_Time", (float)frameIndex_ * 0.016f);
      plasmaShader_->SetFloat("u_Intensity", settings_.PlasmaIntensity);
      plasmaShader_->SetFloat("u_Speed", settings_.PlasmaSpeed);
      plasmaShader_->SetFloat("u_Scale", settings_.PlasmaScale);
      plasmaShader_->SetVec3("u_ColorA", settings_.PlasmaColorA);
      plasmaShader_->SetVec3("u_ColorB", settings_.PlasmaColorB);
      plasmaShader_->SetVec3("u_ColorC", settings_.PlasmaColorC);
      plasmaShader_->SetBool("u_Enable", settings_.EnablePlasma);

      RenderQuad();
      intermediateB_->Unbind();

      intermediateA_->Bind();
      glClear(GL_COLOR_BUFFER_BIT);
      plasmaShader_->Bind();

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, intermediateB_->GetColorAttachmentRendererID(0));
      plasmaShader_->SetInt("u_Texture", 0);
      plasmaShader_->SetFloat("u_Time", (float)frameIndex_ * 0.016f);
      plasmaShader_->SetFloat("u_Intensity", settings_.PlasmaIntensity);
      plasmaShader_->SetFloat("u_Speed", settings_.PlasmaSpeed);
      plasmaShader_->SetFloat("u_Scale", settings_.PlasmaScale);
      plasmaShader_->SetVec3("u_ColorA", settings_.PlasmaColorA);
      plasmaShader_->SetVec3("u_ColorB", settings_.PlasmaColorB);
      plasmaShader_->SetVec3("u_ColorC", settings_.PlasmaColorC);
      plasmaShader_->SetBool("u_Enable", settings_.EnablePlasma);

      RenderQuad();
      intermediateA_->Unbind();
  }

  void RenderSystem::ExecuteSSSSPass(World& world) {
      (void)world;
      if (!ssssShader_ || !ssssFBO_ || !gBuffer_ || !intermediateA_) return;

      ssssFBO_->Bind();
      glClear(GL_COLOR_BUFFER_BIT);

      ssssShader_->Bind();

      // Bind G-Buffer textures
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, gBuffer_->GetColorAttachmentRendererID(0)); // Position
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, gBuffer_->GetColorAttachmentRendererID(2)); // Normal + Roughness
      glActiveTexture(GL_TEXTURE2);
      glBindTexture(GL_TEXTURE_2D, gBuffer_->GetColorAttachmentRendererID(1)); // Albedo
      glActiveTexture(GL_TEXTURE3);
      glBindTexture(GL_TEXTURE_2D, gBuffer_->GetColorAttachmentRendererID(4)); // Subsurface

      ssssShader_->SetInt("gPosition", 0);
      ssssShader_->SetInt("gNormal", 1);
      ssssShader_->SetInt("gAlbedo", 2);
      ssssShader_->SetInt("gSubsurface", 3);

      ssssShader_->SetMat4("projection", camera3D_->GetProjectionMatrix());
      ssssShader_->SetMat4("view", camera3D_->GetViewMatrix());
      ssssShader_->SetVec3("cameraPos", camera3D_->GetPosition());
      ssssShader_->SetVec2("viewportSize", Math::Vec2f(
          (float)ssssFBO_->GetSpecification().Width,
          (float)ssssFBO_->GetSpecification().Height));

      ssssShader_->SetFloat("u_Power", settings_.SSSSPower);
      ssssShader_->SetFloat("u_Scale", 1.0f);
      ssssShader_->SetFloat("u_Radius", settings_.SSSSRadius);
      ssssShader_->SetFloat("u_Intensity", settings_.SSSSIntensity);
      ssssShader_->SetInt("u_SampleCount", settings_.SSSSSampleCount);

      RenderQuad();
      ssssFBO_->Unbind();

      // Blend SSS result back into intermediateA_
      intermediateA_->Bind();
      glEnable(GL_BLEND);
      glBlendFunc(GL_ONE, GL_ONE); // Additive blending

      static std::shared_ptr<renderer::Shader> blendShader = renderer::Shader::Create("./src/shaders/postprocess.vert.glsl", "./src/shaders/postprocess.frag.glsl");
      blendShader->Bind();

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, ssssFBO_->GetColorAttachmentRendererID(0));
      blendShader->SetInt("tex", 0);

      RenderQuad();

      glDisable(GL_BLEND);
      intermediateA_->Unbind();
  }

  void RenderSystem::ExecuteRefractionPass(World& world) {
      (void)world;
      if (!refractionShader_ || !refractionFBO_ || !gBuffer_ || !intermediateA_) return;

      refractionFBO_->Bind();
      glClear(GL_COLOR_BUFFER_BIT);

      refractionShader_->Bind();

      // Bind G-Buffer textures
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, gBuffer_->GetColorAttachmentRendererID(0)); // Position
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, gBuffer_->GetColorAttachmentRendererID(2)); // Normal + Roughness
      glActiveTexture(GL_TEXTURE3);
      glBindTexture(GL_TEXTURE_2D, gBuffer_->GetColorAttachmentRendererID(4)); // Subsurface

      refractionShader_->SetInt("gPosition", 0);
      refractionShader_->SetInt("gNormal", 1);
      refractionShader_->SetInt("gSubsurface", 3);

      // Bind scene color
      glActiveTexture(GL_TEXTURE4);
      glBindTexture(GL_TEXTURE_2D, intermediateA_->GetColorAttachmentRendererID(0));
      refractionShader_->SetInt("u_SceneColor", 4);

      refractionShader_->SetMat4("projection", camera3D_->GetProjectionMatrix());
      refractionShader_->SetMat4("view", camera3D_->GetViewMatrix());
      refractionShader_->SetMat4("invView", camera3D_->GetViewMatrix().Inverse());
      refractionShader_->SetMat4("invProj", camera3D_->GetProjectionMatrix().Inverse());
      refractionShader_->SetVec3("cameraPos", camera3D_->GetPosition());
      refractionShader_->SetVec2("viewportSize", Math::Vec2f(
          (float)refractionFBO_->GetSpecification().Width,
          (float)refractionFBO_->GetSpecification().Height));

      refractionShader_->SetFloat("u_IOR", 1.5f);
      refractionShader_->SetFloat("u_Thickness", 1.0f);
      refractionShader_->SetVec3("u_TintColor", Math::Vec3f(1.0f, 1.0f, 1.0f));
      refractionShader_->SetFloat("u_Intensity", settings_.RefractionIntensity);

      RenderQuad();
      refractionFBO_->Unbind();

      // Copy refraction result back to intermediateA_
      intermediateA_->Bind();
      static std::shared_ptr<renderer::Shader> copyShader = renderer::Shader::Create("./src/shaders/postprocess.vert.glsl", "./src/shaders/postprocess.frag.glsl");
      copyShader->Bind();

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, refractionFBO_->GetColorAttachmentRendererID(0));
      copyShader->SetInt("tex", 0);

      RenderQuad();
      intermediateA_->Unbind();
  }


  void RenderSystem::ExecuteSSAOPass(World& world) {
    if (!camera3D_ || !gBuffer_ || !ssaoShader_) return;
    ScopedProfileTimer ssaoTimer(&renderer::Renderer2D::GetStats().PassSSAO);

     // 1. G-Buffer Pass
     gBuffer_->Bind();
     glClearColor(0, 0, 0, 0);
     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

     renderer::RendererAPI::GetAPI(); // Keep reference to avoid unused warning if needed, but we use direct GL here
     glEnable(GL_DEPTH_TEST);
     gBufferShader_->Bind();
     
     // Render all meshes to G-Buffer
     auto meshEntities = world.Query<ge::ecs::TagComponent>(); // Temporary query for all renderables
     for (auto entity : meshEntities) {
         if (!world.HasMesh(entity) || !world.HasComponent<ge::ecs::TransformComponent>(entity)) continue;
         
         auto& tc = world.GetTransform(entity);
         gBufferShader_->SetMat4("u_Model", tc.GetTransform());
         
         // Pass TAA temporal data
         gBufferShader_->SetMat4("u_PrevViewProj", prevViewProj_);
         
         Math::Mat4f prevModel = tc.GetTransform(); // Default to current
         if (prevModelMatrices_.find(entity) != prevModelMatrices_.end()) {
             prevModel = prevModelMatrices_[entity];
         }
         gBufferShader_->SetMat4("u_PrevModel", prevModel);
         
         // Store current for next frame
         prevModelMatrices_[entity] = tc.GetTransform();
         
         // Set material properties for G-Buffer
         auto& mc = world.GetMesh(entity);
         if (mc.MaterialPtr) {
             gBufferShader_->SetVec4("u_Albedo", vec4(mc.AlbedoColor, mc.Metallic));
             gBufferShader_->SetVec2("u_Material", vec2(mc.Roughness, 0.0f)); // Roughness in X, AO in Y (unused)
         } else {
             gBufferShader_->SetVec4("u_Albedo", vec4(1.0f, 1.0f, 1.0f, 0.0f)); // Default white, no metallic
             gBufferShader_->SetVec2("u_Material", vec2(0.5f, 0.0f)); // Default roughness
         }
         
         if (mc.MeshPtr) {
             mc.MeshPtr->Draw();
         }
     }
     
     // 1.5 Decal Pass - Apply decals to G-Buffer
     if (!decalEntities.empty() && decalShader_) {
         // Bind to G-Buffer for read/write
         gBuffer_->Bind();
         
         // Disable depth test for decal projection but keep depth writes off
         glDisable(GL_DEPTH_TEST);
         glDepthMask(GL_FALSE);
         
         // Use blending for decals
         glEnable(GL_BLEND);
         glBlendEquation(GL_FUNC_ADD);
         glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
         
         decalShader_->Bind();
         
         // Set up common uniforms
         decalShader_->SetMat4("u_Projection", camera3D_->GetProjectionMatrix());
         decalShader_->SetFloat("u_FadeStart", 0.0f); // These would come from decal component
         decalShader_->SetFloat("u_FadeEnd", 1.0f);
         
         // Bind G-Buffer textures for reading
         glActiveTexture(GL_TEXTURE0);
         glBindTexture(GL_TEXTURE_2D, gBuffer_->GetColorAttachmentRendererID(0)); // Position
         glActiveTexture(GL_TEXTURE1);
         glBindTexture(GL_TEXTURE_2D, gBuffer_->GetColorAttachmentRendererID(1)); // Albedo
         glActiveTexture(GL_TEXTURE2);
         glBindTexture(GL_TEXTURE_2D, gBuffer_->GetColorAttachmentRendererID(2)); // Normal
         glActiveTexture(GL_TEXTURE3);
         glBindTexture(GL_TEXTURE_2D, gBuffer_->GetColorAttachmentRendererID(3)); // Velocity
         
         decalShader_->SetInt("gPosition", 0);
         decalShader_->SetInt("gAlbedo", 1);
         decalShader_->SetInt("gNormal", 2);
         decalShader_->SetInt("gVelocity", 3);
         
         // Render each decal as a fullscreen quad (in a real implementation, we'd render bounding volumes)
         for (auto const& entity : decalEntities) {
             if (!world.HasDecal(entity)) continue;
             
             auto& decal = world.GetDecal(entity);
             if (!decal.Enabled || !decal.Albedo) continue;
             
             // Set decal-specific uniforms
             decalShader_->SetMat4("u_Model", Math::Mat4f::Identity()); // Simplified
             decalShader_->SetVec4("u_DecalAlbedoColor", vec4(1.0f)); // Would use decal.Albedo color
             decalShader_->SetVec2("u_DecalMaterial", vec2(0.5f, 0.0f)); // Would use decal material
             
             // Bind decal textures
             glActiveTexture(GL_TEXTURE4);
             glBindTexture(GL_TEXTURE_2D, decal.Albedo->GetID());
             decalShader_->SetInt("u_DecalAlbedo", 4);
             
             glActiveTexture(GL_TEXTURE5);
             glBindTexture(GL_TEXTURE_2D, decal.Normal ? decal.Normal->GetID() : 0);
             decalShader_->SetInt("u_DecalNormal", 5);
             
             // Render fullscreen quad
             RenderQuad();
         }
         
         // Restore depth state
         glDisable(GL_BLEND);
         glDepthMask(GL_TRUE);
         glEnable(GL_DEPTH_TEST);
         
         gBuffer_->Unbind();
     }
     
     gBuffer_->Unbind();

    // 2. SSAO Pass
    ssaoFBO_->Bind();
    glClear(GL_COLOR_BUFFER_BIT);
    ssaoShader_->Bind();
    
    // Bind G-Buffer textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gBuffer_->GetColorAttachmentRendererID(0)); // Position
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gBuffer_->GetColorAttachmentRendererID(1)); // Normal
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, ssaoNoiseTexture_);
    
    ssaoShader_->SetInt("gPosition", 0);
    ssaoShader_->SetInt("gNormal", 1);
    ssaoShader_->SetInt("texNoise", 2);
    ssaoShader_->SetMat4("projection", camera3D_->GetProjectionMatrix());
    
    // Set kernel samples
    for (unsigned int i = 0; i < 64; ++i) {
        ssaoShader_->SetVec3("samples[" + std::to_string(i) + "]", ssaoKernel_[i]);
    }
    
    ssaoShader_->SetInt("kernelSize", settings_.SSAOKernelSize);
    ssaoShader_->SetFloat("radius", settings_.SSAORadius);
    ssaoShader_->SetFloat("bias", settings_.SSAOBias);
    ssaoShader_->SetFloat("intensity", settings_.SSAOIntensity);
    
    RenderQuad();
    ssaoFBO_->Unbind();

     // 3. SSAO Blur Pass
     ssaoBlurFBO_->Bind();
     glClear(GL_COLOR_BUFFER_BIT);
     ssaoBlurShader_->Bind();
     
     glActiveTexture(GL_TEXTURE0);
     glBindTexture(GL_TEXTURE_2D, ssaoFBO_->GetColorAttachmentRendererID(0));
     ssaoBlurShader_->SetInt("ssaoInput", 0);
     
     RenderQuad();
     ssaoBlurFBO_->Unbind();
     
     // 4. SSGI Pass (Screen Space Global Illumination)
     if (settings_.EnableSSGI && ssgiFBO_ && ssgiShader_) {
         ScopedProfileTimer ssgiTimer(&renderer::Renderer2D::GetStats().PassSSGI);
         
         ssgiFBO_->Bind();
         glClear(GL_COLOR_BUFFER_BIT);
         
         ssgiShader_->Bind();
         
         // Bind G-Buffer textures
         glActiveTexture(GL_TEXTURE0);
         glBindTexture(GL_TEXTURE_2D, gBuffer_->GetColorAttachmentRendererID(0)); // Position
         glActiveTexture(GL_TEXTURE1);
         glBindTexture(GL_TEXTURE_2D, gBuffer_->GetColorAttachmentRendererID(1)); // Normal
         glActiveTexture(GL_TEXTURE2);
         glBindTexture(GL_TEXTURE_2D, gBuffer_->GetColorAttachmentRendererID(2)); // Albedo
         glActiveTexture(GL_TEXTURE3);
         glBindTexture(GL_TEXTURE_2D, ssaoNoiseTexture_); // Noise for sampling
         
         ssgiShader_->SetInt("gPosition", 0);
         ssgiShader_->SetInt("gNormal", 1);
         ssgiShader_->SetInt("gAlbedo", 2);
         ssgiShader_->SetInt("texNoise", 3);
         ssgiShader_->SetMat4("projection", camera3D_->GetProjectionMatrix());
         ssgiShader_->SetMat4("view", camera3D_->GetViewMatrix());
         ssgiShader_->SetVec3("cameraPos", camera3D_->GetPosition());
         
         // SSGI parameters
         ssgiShader_->SetInt("sampleCount", settings_.SSGISampleCount);
         ssgiShader_->SetFloat("radius", settings_.SSGIRadius);
         ssgiShader_->SetFloat("intensity", settings_.SSGIIntensity);
         ssgiShader_->SetFloat("bounceIntensity", settings_.SSGIBounceIntensity);
         
          RenderQuad();
          ssgiFBO_->Unbind();
      }

      // 5. SSR Pass (Screen Space Reflections)
      if (settings_.EnableSSR && ssrFBO_ && ssrShader_) {
          ScopedProfileTimer ssrTimer(&renderer::Renderer2D::GetStats().PassSSR);
          
          ssrFBO_->Bind();
          glClear(GL_COLOR_BUFFER_BIT);
          
          ssrShader_->Bind();
          
          // Bind G-Buffer textures
          glActiveTexture(GL_TEXTURE0);
          glBindTexture(GL_TEXTURE_2D, gBuffer_->GetColorAttachmentRendererID(0)); // Position (view space)
          glActiveTexture(GL_TEXTURE1);
          glBindTexture(GL_TEXTURE_2D, gBuffer_->GetColorAttachmentRendererID(2)); // Normal (RGB) + Roughness (A)
          glActiveTexture(GL_TEXTURE2);
          glBindTexture(GL_TEXTURE_2D, gBuffer_->GetColorAttachmentRendererID(1)); // Albedo
          
          ssrShader_->SetInt("gPosition", 0);
          ssrShader_->SetInt("gNormal", 1);
          ssrShader_->SetInt("gAlbedo", 2);
          
          // Camera and projection matrices
          ssrShader_->SetMat4("projection", camera3D_->GetProjectionMatrix());
          ssrShader_->SetMat4("view", camera3D_->GetViewMatrix());
          ssrShader_->SetMat4("invView", camera3D_->GetViewMatrix().Inverse());
          ssrShader_->SetMat4("invProj", camera3D_->GetProjectionMatrix().Inverse());
          ssrShader_->SetVec3("cameraPos", camera3D_->GetPosition());
          ssrShader_->SetVec2("viewportSize", Math::Vec2f(width, height));
          
          // SSR parameters
          ssrShader_->SetInt("steps", settings_.SSRSteps);
          ssrShader_->SetFloat("stepSize", settings_.SSRStepSize);
          ssrShader_->SetFloat("fadeDistance", settings_.SSRFadeDistance);
          ssrShader_->SetFloat("thickness", settings_.SSRThickness);
          ssrShader_->SetFloat("roughnessFade", settings_.SSRRoughnessFade);
          
          RenderQuad();
          ssrFBO_->Unbind();
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

  // ============================================================================
  // Forward+ Clustered Lighting Implementation
  // ============================================================================

  void RenderSystem::BuildForwardPlusClusters() {
      if (!camera3D_) return;

      clusterCountX_ = settings_.ClusterSizeX;
      clusterCountY_ = settings_.ClusterSizeY;
      clusterCountZ_ = settings_.ClusterSizeZ;
      maxLightsPerCluster_ = settings_.MaxLightsPerCluster;
      maxLights_ = settings_.MaxLights;

      // Initialize GPU buffers if needed
      if (lightBuffer_ == 0) {
          glGenBuffers(1, &lightBuffer_);
      }
      if (clusterLightIndicesBuffer_ == 0) {
          glGenBuffers(1, &clusterLightIndicesBuffer_);
      }

      // Allocate cluster data
      int totalClusters = clusterCountX_ * clusterCountY_ * clusterCountZ_;
      
      fpData_.lightPositions = new Math::Vec3f[maxLights_];
      fpData_.lightColors = new Math::Vec3f[maxLights_];
      fpData_.lightIntensities = new float[maxLights_];
      fpData_.lightRanges = new float[maxLights_];
      fpData_.lightTypes = new int[maxLights_];
      fpData_.lightDirections = new Math::Vec3f[maxLights_];
      fpData_.lightSpotOuter = new float[maxLights_];
      fpData_.lightSpotInner = new float[maxLights_];
      fpData_.lightIndicesPerCluster = new int[totalClusters * maxLightsPerCluster_];
      fpData_.lightCountPerCluster = new int[totalClusters];
      fpData_.clusterBounds = new Math::Vec4f[totalClusters * 2];

      // Initialize light counts to 0
      memset(fpData_.lightCountPerCluster, 0, sizeof(int) * totalClusters);
      memset(fpData_.lightIndicesPerCluster, -1, sizeof(int) * totalClusters * maxLightsPerCluster_);
  }

  void RenderSystem::AssignLightsToClusters(const std::vector<ecs::Entity>& lightEntities, World& world) {
      if (!camera3D_ || lightEntities.empty()) return;

      int totalClusters = clusterCountX_ * clusterCountY_ * clusterCountZ_;
      float nearPlane = camera3D_->GetNearPlane();
      float farPlane = camera3D_->GetFarPlane();

      // Get camera matrices
      Math::Mat4f view = camera3D_->GetViewMatrix();
      Math::Mat4f proj = camera3D_->GetProjectionMatrix();
      Math::Mat4f viewProj = proj * view;
      Math::Vec3f camPos = camera3D_->GetPosition();

      // Collect light data
      int lightCount = std::min((int)lightEntities.size(), maxLights_);
      
      for (int i = 0; i < lightCount; ++i) {
          auto& lc = world.GetLight(lightEntities[i]);
          auto& lt = world.GetTransform(lightEntities[i]);

          fpData_.lightPositions[i] = lt.position;
          fpData_.lightColors[i] = lc.Color;
          fpData_.lightIntensities[i] = lc.Intensity;
          fpData_.lightRanges[i] = lc.Range;
          fpData_.lightTypes[i] = (int)lc.Type;

          Math::Vec4f dir4 = lt.rotation.ToMat4x4() * Math::Vec4f(0, 0, -1, 0);
          fpData_.lightDirections[i] = { dir4.x, dir4.y, dir4.z };
          fpData_.lightSpotOuter[i] = lc.SpotOuterCone;
          fpData_.lightSpotInner[i] = lc.SpotInnerCone;

          // Assign light to clusters based on its position and range
          float lightRange = lc.Range;
          if (lc.Type == LightType::Directional) {
              lightRange = farPlane * 2.0f; // Directional lights affect everything
          }

          // Determine which clusters this light affects
          Math::Vec3f lightPos = lt.position;

          // Simple bounding sphere to cluster assignment
          for (int z = 0; z < clusterCountZ_; ++z) {
              for (int y = 0; y < clusterCountY_; ++y) {
                  for (int x = 0; x < clusterCountX_; ++x) {
                      int clusterIdx = z * clusterCountX_ * clusterCountY_ + y * clusterCountX_ + x;

                      // Calculate cluster bounds in view space (simplified)
                      float zNear = nearPlane + (float(z) / clusterCountZ_) * (farPlane - nearPlane);
                      float zFar = nearPlane + (float(z + 1) / clusterCountZ_) * (farPlane - nearPlane);
                      
                      // Check if light affects this cluster
                      bool affects = false;
                      if (lc.Type == LightType::Directional) {
                          affects = true; // Directional lights affect all clusters
                      } else {
                          // Point/Spot lights - check distance to cluster center
                          float clusterZ = (zNear + zFar) * 0.5f;
                          Math::Vec3f clusterCenter = camPos + Math::Vec3f(0, 0, -clusterZ);
                          float dist = (lightPos - clusterCenter).Length();
                          if (dist < lightRange) {
                              affects = true;
                          }
                      }

                      if (affects && fpData_.lightCountPerCluster[clusterIdx] < maxLightsPerCluster_) {
                          fpData_.lightIndicesPerCluster[clusterIdx * maxLightsPerCluster_ + fpData_.lightCountPerCluster[clusterIdx]] = i;
                          fpData_.lightCountPerCluster[clusterIdx]++;
                      }
                  }
              }
          }
      }

      // Upload light data to GPU
      UploadLightDataToGPU(lightCount);
  }

  void RenderSystem::UploadLightDataToGPU(int lightCount) {
      if (lightBuffer_ == 0 || lightCount <= 0) return;

      // Pack light data into struct matching shader
      struct PackedLight {
          Math::Vec3f Position;
          float Range;
          Math::Vec3f Color;
          float Intensity;
          Math::Vec3f Direction;
          float SpotOuterCone;
          float SpotInnerCone;
          int Type;
          float Padding;
      };

      std::vector<PackedLight> packedLights(lightCount);
      for (int i = 0; i < lightCount; ++i) {
          packedLights[i].Position = fpData_.lightPositions[i];
          packedLights[i].Range = fpData_.lightRanges[i];
          packedLights[i].Color = fpData_.lightColors[i];
          packedLights[i].Intensity = fpData_.lightIntensities[i];
          packedLights[i].Direction = fpData_.lightDirections[i];
          packedLights[i].SpotOuterCone = fpData_.lightSpotOuter[i];
          packedLights[i].SpotInnerCone = fpData_.lightSpotInner[i];
          packedLights[i].Type = fpData_.lightTypes[i];
          packedLights[i].Padding = 0.0f;
      }

      // Upload light buffer
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightBuffer_);
      glBufferData(GL_SHADER_STORAGE_BUFFER, packedLights.size() * sizeof(PackedLight), packedLights.data(), GL_DYNAMIC_DRAW);
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

      // Upload cluster light indices
      int totalClusters = clusterCountX_ * clusterCountY_ * clusterCountZ_;
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, clusterLightIndicesBuffer_);
      glBufferData(GL_SHADER_STORAGE_BUFFER, totalClusters * maxLightsPerCluster_ * sizeof(int), 
                   fpData_.lightIndicesPerCluster, GL_DYNAMIC_DRAW);
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

      // Upload cluster light counts
      if (clusterLightCountsBuffer_ == 0) {
          glGenBuffers(1, &clusterLightCountsBuffer_);
      }
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, clusterLightCountsBuffer_);
      glBufferData(GL_SHADER_STORAGE_BUFFER, totalClusters * sizeof(int), 
                   fpData_.lightCountPerCluster, GL_DYNAMIC_DRAW);
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
  }

} // namespace ecs
} // namespace ge
