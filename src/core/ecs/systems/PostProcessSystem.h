#pragma once

#include "../../renderer/Framebuffer.h"
#include "../../renderer/Shader.h"
#include "../System.h"
#include "../World.h"
#include "../../renderer/SSGIPass.h"
#include <memory>

namespace ge {
namespace ecs {

    class PostProcessSystem : public System {
    public:
        PostProcessSystem();
        ~PostProcessSystem() = default;

        void Init(uint32_t width, uint32_t height);
        void Resize(uint32_t width, uint32_t height);
        
        /**
         * @brief Processes the input framebuffer and returns the resulting texture ID.
         */
        uint32_t Process(World& world, std::shared_ptr<renderer::Framebuffer> inputFB);

    private:
        std::shared_ptr<renderer::Framebuffer> m_ThresholdFB;
        std::shared_ptr<renderer::Framebuffer> m_BlurFBs[2];
        std::shared_ptr<renderer::Framebuffer> m_FinalFB;
        std::shared_ptr<renderer::Framebuffer> m_SSGIFB; // SSGI pass output

        std::shared_ptr<renderer::Shader> m_ThresholdShader;
        std::shared_ptr<renderer::Shader> m_BlurShader;
        std::shared_ptr<renderer::Shader> m_CompositeShader;
        std::shared_ptr<renderer::SSGIPass> m_SSGIPass; // SSGI pass

        uint32_t m_Width = 1280, m_Height = 720;
    };

} // namespace ecs
} // namespace ge
