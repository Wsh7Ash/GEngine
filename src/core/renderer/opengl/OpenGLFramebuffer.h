#pragma once

#include "../Framebuffer.h"

namespace ge {
namespace renderer {

    class OpenGLFramebuffer : public Framebuffer
    {
    public:
        OpenGLFramebuffer(const FramebufferSpecification& spec);
        virtual ~OpenGLFramebuffer();

        void Invalidate();

        virtual void Bind() override;
        virtual void Unbind() override;

        virtual void Resize(uint32_t width, uint32_t height) override;

        virtual uint32_t GetColorAttachmentRendererID() const override { return colorAttachment_; }
        virtual const FramebufferSpecification& GetSpecification() const override { return spec_; }

    private:
        uint32_t rendererID_ = 0;
        uint32_t colorAttachment_ = 0;
        uint32_t depthAttachment_ = 0;
        FramebufferSpecification spec_;
    };

} // namespace renderer
} // namespace ge
