#include "PostProcessSystem.h"
#include "../components/PostProcessComponent.h"
#include "../../renderer/Renderer2D.h"
#include "../../../../deps/glad/include/glad/glad.h"

namespace ge {
namespace ecs {

    PostProcessSystem::PostProcessSystem() {
        m_ThresholdShader = renderer::Shader::Create("assets/shaders/PostProcessThreshold.glsl");
        m_BlurShader = renderer::Shader::Create("assets/shaders/PostProcessBlur.glsl");
        m_CompositeShader = renderer::Shader::Create("assets/shaders/PostProcessComposite.glsl");
    }

    void PostProcessSystem::Init(uint32_t width, uint32_t height) {
        m_Width = width;
        m_Height = height;
        
        renderer::FramebufferSpecification spec;
        spec.Width = width;
        spec.Height = height;
        spec.Attachments = { renderer::FramebufferTextureFormat::RGBA16F };
        
        m_ThresholdFB = renderer::Framebuffer::Create(spec);
        m_BlurFBs[0] = renderer::Framebuffer::Create(spec);
        m_BlurFBs[1] = renderer::Framebuffer::Create(spec);
        m_FinalFB = renderer::Framebuffer::Create(spec);
        m_SSGIFB = renderer::Framebuffer::Create(spec); // SSGI pass output
    }

    void PostProcessSystem::Resize(uint32_t width, uint32_t height) {
        if (width == 0 || height == 0) return;
        m_Width = width;
        m_Height = height;

        m_ThresholdFB->Resize(width, height);
        m_BlurFBs[0]->Resize(width, height);
        m_BlurFBs[1]->Resize(width, height);
        m_FinalFB->Resize(width, height);
    }

    uint32_t PostProcessSystem::Process(World& world, std::shared_ptr<renderer::Framebuffer> inputFB) {
        auto entities = world.Query<PostProcessComponent>();
        if (entities.begin() == entities.end()) return inputFB->GetColorAttachmentRendererID(0);
        
        auto& ppc = world.GetComponent<PostProcessComponent>(*entities.begin());
        if (!ppc.Enabled) return inputFB->GetColorAttachmentRendererID(0);
        
        // 0. SSGI Pass (Screen Space Global Illumination)
        // This needs the G-buffer from the rendering system
        // For now, we'll skip this as we don't have direct access to G-buffer
        // In a full implementation, this would come after G-buffer generation
        // We would need to modify the rendering pipeline to provide G-buffer to post-processing
        
        // 1. Threshold Pass (Extract bright parts)
        m_ThresholdFB->Bind();
        glClear(GL_COLOR_BUFFER_BIT);
        m_ThresholdShader->Bind();
        m_ThresholdShader->SetFloat("u_Threshold", ppc.BloomThreshold);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, inputFB->GetColorAttachmentRendererID(0));
        renderer::Renderer2D::DrawFullscreenQuad();
        m_ThresholdFB->Unbind();
        
        // 2. Blur Passes (Gaussian)
        bool horizontal = true, first_iteration = true;
        int amount = 10;
        m_BlurShader->Bind();
        for (int i = 0; i < amount; i++) {
            m_BlurFBs[horizontal]->Bind();
            m_BlurShader->SetInt("u_Horizontal", horizontal);
            glBindTexture(GL_TEXTURE_2D, first_iteration ? m_ThresholdFB->GetColorAttachmentRendererID(0) : m_BlurFBs[!horizontal]->GetColorAttachmentRendererID(0));
            renderer::Renderer2D::DrawFullscreenQuad();
            horizontal = !horizontal;
            if (first_iteration) first_iteration = false;
        }
        m_BlurFBs[0]->Unbind();
        
        // 3. Final Composite Pass
        m_FinalFB->Bind();
        glClear(GL_COLOR_BUFFER_BIT);
        m_CompositeShader->Bind();
        
        // Basic exposure & tone mapping
        m_CompositeShader->SetFloat("u_Exposure", ppc.Exposure);
        m_CompositeShader->SetFloat("u_WhitePoint", ppc.WhitePoint);
        m_CompositeShader->SetInt("u_ToneMappingType", ppc.ToneMappingType);
        
        // Auto-exposure
        m_CompositeShader->SetFloat("u_AutoExposure", ppc.AutoExposure ? m_CurrentExposure : 0.0f);
        
        // Color grading
        m_CompositeShader->SetFloat("u_Gamma", ppc.Gamma);
        
        // Bloom
        m_CompositeShader->SetFloat("u_BloomIntensity", ppc.BloomIntensity);
        
        // Vignette
        m_CompositeShader->SetBool("u_VignetteEnabled", ppc.VignetteEnabled);
        m_CompositeShader->SetFloat("u_VignetteIntensity", ppc.VignetteIntensity);
        m_CompositeShader->SetFloat("u_VignetteSmoothness", ppc.VignetteSmoothness);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, inputFB->GetColorAttachmentRendererID(0));
        m_CompositeShader->SetInt("u_SceneTexture", 0);
        
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_BlurFBs[!horizontal]->GetColorAttachmentRendererID(0));
        m_CompositeShader->SetInt("u_BloomTexture", 1);
        
        renderer::Renderer2D::DrawFullscreenQuad();
        m_FinalFB->Unbind();
        
        return m_FinalFB->GetColorAttachmentRendererID(0);
    }

} // namespace ecs
} // namespace ge
