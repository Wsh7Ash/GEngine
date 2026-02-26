#pragma once

#include "../renderer/Framebuffer.h"
#include "../math/VecTypes.h"
#include <memory>

namespace ge {
namespace editor {

    /**
     * @brief Panel that displays the rendered Framebuffer texture.
     */
    class ViewportPanel
    {
    public:
        ViewportPanel();
        ~ViewportPanel() = default;

        void OnImGuiRender();

        std::shared_ptr<renderer::Framebuffer> GetFramebuffer() { return framebuffer_; }

    private:
        std::shared_ptr<renderer::Framebuffer> framebuffer_;
        bool isFocused_ = false;
        bool isHovered_ = false;
        Math::Vec2f viewportSize_ = { 0, 0 };
    };

} // namespace editor
} // namespace ge
