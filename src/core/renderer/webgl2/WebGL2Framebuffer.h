#pragma once

#include "../Framebuffer.h"
#include <vector>

namespace ge {
namespace renderer {

class WebGL2Framebuffer : public Framebuffer {
public:
    WebGL2Framebuffer(const FramebufferSpecification& spec);
    virtual ~WebGL2Framebuffer() override;

    virtual void Bind() override;
    virtual void Unbind() override;
    virtual void Resize(uint32_t width, uint32_t height) override;
    virtual uint32_t GetColorAttachmentRendererID(uint32_t index = 0) const override;
    virtual uint32_t GetDepthAttachmentRendererID() const override;
    virtual uint32_t GetEntityAttachmentRendererID() const override;
    virtual void ClearAttachment(uint32_t attachmentIndex, int value) override;
    virtual int ReadPixel(uint32_t attachmentIndex, int x, int y) override;
    virtual const FramebufferSpecification& GetSpecification() const override { return specification_; }

private:
    void Invalidate();
    void Destroy();

    uint32_t rendererID_ = 0;
    uint32_t colorAttachment_ = 0;
    uint32_t depthAttachment_ = 0;
    uint32_t entityAttachment_ = 0;
    FramebufferSpecification specification_;
};

} // namespace renderer
} // namespace ge
