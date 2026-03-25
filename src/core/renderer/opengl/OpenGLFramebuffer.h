#pragma once

#include "../Framebuffer.h"
#include "../../debug/assert.h"
#include <vector>
#include <cstdint>

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

  virtual uint32_t GetColorAttachmentRendererID(uint32_t index = 0) const override {
    GE_ASSERT(index < colorAttachments_.size(), "Color attachment index out of bounds!");
    return spec_.Samples > 1 ? resolvedColorAttachments_[index] : colorAttachments_[index];
  }
  virtual uint32_t GetDepthAttachmentRendererID() const override { return depthAttachment_; }
  virtual uint32_t GetEntityAttachmentRendererID() const override {
      for (size_t i = 0; i < colorAttachmentSpecifications_.size(); ++i) {
          if (colorAttachmentSpecifications_[i].TextureFormat == FramebufferTextureFormat::RED_INTEGER) {
              return spec_.Samples > 1 ? resolvedColorAttachments_[i] : colorAttachments_[i];
          }
      }
      return 0;
  }
  virtual void ClearAttachment(uint32_t attachmentIndex, int value) override;
  virtual int ReadPixel(uint32_t attachmentIndex, int x, int y) override;
  virtual const FramebufferSpecification &GetSpecification() const override {
    return spec_;
  }

private:
  uint32_t rendererID_ = 0;
  std::vector<uint32_t> colorAttachments_;
  uint32_t depthAttachment_ = 0;

  uint32_t resolvedRendererID_ = 0;
  std::vector<uint32_t> resolvedColorAttachments_;
  FramebufferSpecification spec_;

  std::vector<FramebufferTextureSpecification> colorAttachmentSpecifications_;
  FramebufferTextureSpecification depthAttachmentSpecification_ = FramebufferTextureFormat::None;
};

} // namespace renderer
} // namespace ge
