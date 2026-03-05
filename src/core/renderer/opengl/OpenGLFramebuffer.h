#pragma once

#include "../Framebuffer.h"

namespace ge {
namespace renderer {

class OpenGLFramebuffer : public Framebuffer {
public:
  OpenGLFramebuffer(const FramebufferSpecification &spec);
  virtual ~OpenGLFramebuffer();

  void Invalidate();

  virtual void Bind() override;
  virtual void Unbind() override;

  virtual void Resize(uint32_t width, uint32_t height) override;

  virtual uint32_t GetColorAttachmentRendererID() const override {
    return spec_.Samples > 1 ? resolvedColorAttachment_ : colorAttachment_;
  }
  virtual uint32_t GetEntityAttachmentRendererID() const override {
    return spec_.Samples > 1 ? resolvedEntityAttachment_ : entityAttachment_;
  }
  virtual void ClearAttachment(uint32_t attachmentIndex, int value) override;
  virtual int ReadPixel(uint32_t attachmentIndex, int x, int y) override;
  virtual const FramebufferSpecification &GetSpecification() const override {
    return spec_;
  }

private:
  uint32_t rendererID_ = 0;
  uint32_t colorAttachment_ = 0;
  uint32_t entityAttachment_ = 0;
  uint32_t depthAttachment_ = 0;

  uint32_t resolvedRendererID_ = 0;
  uint32_t resolvedColorAttachment_ = 0;
  uint32_t resolvedEntityAttachment_ = 0;
  FramebufferSpecification spec_;
};

} // namespace renderer
} // namespace ge
