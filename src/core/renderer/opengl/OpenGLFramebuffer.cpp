#include "OpenGLFramebuffer.h"
#include "../../debug/assert.h"
#include "../debug/log.h"
#include <glad/glad.h>

namespace ge {
namespace renderer {

namespace Utils {

static GLenum TextureTarget(bool multisampled) {
  return multisampled ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
}

static void CreateTextures(bool multisampled, uint32_t *outID, uint32_t count) {
  glCreateTextures(TextureTarget(multisampled), count, outID);
}

static void BindTexture(bool multisampled, uint32_t id) {
  glBindTexture(TextureTarget(multisampled), id);
}

static void AttachColorTexture(uint32_t id, int samples, GLenum internalFormat,
                               GLenum format, uint32_t width, uint32_t height,
                               int index) {
  bool multisampled = samples > 1;
  if (multisampled) {
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, internalFormat,
                            width, height, GL_FALSE);
  } else {
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format,
                 GL_UNSIGNED_BYTE, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index,
                         TextureTarget(multisampled), id, 0);
}

static void AttachDepthTexture(uint32_t id, int samples, GLenum format,
                               GLenum attachmentType, uint32_t width,
                               uint32_t height) {
  bool multisampled = samples > 1;
  if (multisampled) {
    glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, format, width,
                              height, GL_FALSE);
  } else {
    glTexStorage2D(GL_TEXTURE_2D, 1, format, width, height);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }

  glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType,
                         TextureTarget(multisampled), id, 0);
}

static bool IsDepthFormat(FramebufferTextureFormat format) {
  switch (format) {
  case FramebufferTextureFormat::DEPTH24STENCIL8:
    return true;
  }

  return false;
}

static GLenum GETextureFormatToGL(FramebufferTextureFormat format) {
  switch (format) {
  case FramebufferTextureFormat::RGBA8:
    return GL_RGBA8;
  case FramebufferTextureFormat::RGBA16F:
    return GL_RGBA16F;
  case FramebufferTextureFormat::RED_INTEGER:
    return GL_R32I;
  }

  GE_ASSERT(false, "Unknown texture format!");
  return 0;
}

} // namespace Utils

OpenGLFramebuffer::OpenGLFramebuffer(const FramebufferSpecification &spec)
    : spec_(spec) {
  for (auto spec : spec_.Attachments.Attachments) {
    if (!Utils::IsDepthFormat(spec.TextureFormat))
      colorAttachmentSpecifications_.push_back(spec);
    else
      depthAttachmentSpecification_ = spec;
  }

  Invalidate();
}

OpenGLFramebuffer::~OpenGLFramebuffer() {
  glDeleteFramebuffers(1, &rendererID_);
  glDeleteTextures(colorAttachments_.size(), colorAttachments_.data());
  glDeleteTextures(1, &depthAttachment_);

  if (resolvedRendererID_) {
    glDeleteFramebuffers(1, &resolvedRendererID_);
    glDeleteTextures(resolvedColorAttachments_.size(),
                     resolvedColorAttachments_.data());
  }
}

void OpenGLFramebuffer::Invalidate() {
  if (rendererID_) {
    glDeleteFramebuffers(1, &rendererID_);
    glDeleteTextures(colorAttachments_.size(), colorAttachments_.data());
    glDeleteTextures(1, &depthAttachment_);

    if (resolvedRendererID_) {
      glDeleteFramebuffers(1, &resolvedRendererID_);
      glDeleteTextures(resolvedColorAttachments_.size(),
                       resolvedColorAttachments_.data());
      resolvedRendererID_ = 0;
    }
  }

  bool multisampled = spec_.Samples > 1;

  glCreateFramebuffers(1, &rendererID_);
  glBindFramebuffer(GL_FRAMEBUFFER, rendererID_);

  // Color Attachments
  if (colorAttachmentSpecifications_.size()) {
    colorAttachments_.resize(colorAttachmentSpecifications_.size());
    Utils::CreateTextures(multisampled, colorAttachments_.data(),
                          colorAttachments_.size());

    for (size_t i = 0; i < colorAttachments_.size(); i++) {
      Utils::BindTexture(multisampled, colorAttachments_[i]);
      switch (colorAttachmentSpecifications_[i].TextureFormat) {
      case FramebufferTextureFormat::RGBA8:
        Utils::AttachColorTexture(colorAttachments_[i], spec_.Samples, GL_RGBA8,
                                  GL_RGBA, spec_.Width, spec_.Height, i);
        break;
      case FramebufferTextureFormat::RGBA16F:
        Utils::AttachColorTexture(colorAttachments_[i], spec_.Samples, GL_RGBA16F,
                                  GL_RGBA, spec_.Width, spec_.Height, i);
        break;
      case FramebufferTextureFormat::RED_INTEGER:
        Utils::AttachColorTexture(colorAttachments_[i], spec_.Samples, GL_R32I,
                                  GL_RED_INTEGER, spec_.Width, spec_.Height, i);
        break;
      }
    }
  }

  // Depth Attachment
  if (depthAttachmentSpecification_.TextureFormat !=
      FramebufferTextureFormat::None) {
    Utils::CreateTextures(multisampled, &depthAttachment_, 1);
    Utils::BindTexture(multisampled, depthAttachment_);
    switch (depthAttachmentSpecification_.TextureFormat) {
    case FramebufferTextureFormat::DEPTH24STENCIL8:
      Utils::AttachDepthTexture(depthAttachment_, spec_.Samples,
                                GL_DEPTH24_STENCIL8,
                                GL_DEPTH_STENCIL_ATTACHMENT, spec_.Width,
                                spec_.Height);
      break;
    }
  }

  // Resolve Framebuffer
  if (multisampled) {
    glCreateFramebuffers(1, &resolvedRendererID_);
    glBindFramebuffer(GL_FRAMEBUFFER, resolvedRendererID_);

    resolvedColorAttachments_.resize(colorAttachmentSpecifications_.size());
    Utils::CreateTextures(false, resolvedColorAttachments_.data(),
                          resolvedColorAttachments_.size());

    for (size_t i = 0; i < resolvedColorAttachments_.size(); i++) {
      Utils::BindTexture(false, resolvedColorAttachments_[i]);
      switch (colorAttachmentSpecifications_[i].TextureFormat) {
      case FramebufferTextureFormat::RGBA8:
        Utils::AttachColorTexture(resolvedColorAttachments_[i], 1, GL_RGBA8,
                                  GL_RGBA, spec_.Width, spec_.Height, i);
        break;
      case FramebufferTextureFormat::RGBA16F:
        Utils::AttachColorTexture(resolvedColorAttachments_[i], 1, GL_RGBA16F,
                                  GL_RGBA, spec_.Width, spec_.Height, i);
        break;
      case FramebufferTextureFormat::RED_INTEGER:
        Utils::AttachColorTexture(resolvedColorAttachments_[i], 1, GL_R32I,
                                  GL_RED_INTEGER, spec_.Width, spec_.Height, i);
        break;
      }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, rendererID_);
  }

  if (colorAttachments_.size() > 1) {
    GE_ASSERT(colorAttachments_.size() <= 4, "Too many color attachments!");
    GLenum buffers[4] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
                         GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};
    glDrawBuffers(colorAttachments_.size(), buffers);
  } else if (colorAttachments_.empty()) {
    // Only depth-pass
    glDrawBuffer(GL_NONE);
  }

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    GE_LOG_ERROR("Framebuffer is incomplete!");

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGLFramebuffer::ClearAttachment(uint32_t attachmentIndex, int value) {
  GE_ASSERT(attachmentIndex < colorAttachments_.size(), "Attachment index out of bounds!");

  auto &spec = colorAttachmentSpecifications_[attachmentIndex];
  glClearBufferiv(GL_COLOR, attachmentIndex, &value);
}

int OpenGLFramebuffer::ReadPixel(uint32_t attachmentIndex, int x, int y) {
  GE_ASSERT(attachmentIndex < colorAttachments_.size(), "Attachment index out of bounds!");

  if (spec_.Samples > 1) {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, resolvedRendererID_);
  } else {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, rendererID_);
  }

  glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex);
  int pixelData = -1;
  glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &pixelData);
  return pixelData;
}

void OpenGLFramebuffer::Bind() {
  glBindFramebuffer(GL_FRAMEBUFFER, rendererID_);
  glViewport(0, 0, spec_.Width, spec_.Height);
}

void OpenGLFramebuffer::Unbind() {
  if (spec_.Samples > 1) {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, rendererID_);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolvedRendererID_);

    for (size_t i = 0; i < colorAttachments_.size(); i++) {
      glReadBuffer(GL_COLOR_ATTACHMENT0 + i);
      glDrawBuffer(GL_COLOR_ATTACHMENT0 + i);
      glBlitFramebuffer(0, 0, spec_.Width, spec_.Height, 0, 0, spec_.Width,
                        spec_.Height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGLFramebuffer::Resize(uint32_t width, uint32_t height) {
  spec_.Width = width;
  spec_.Height = height;
  Invalidate();
}

} // namespace renderer
} // namespace ge
