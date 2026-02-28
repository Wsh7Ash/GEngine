#include "OpenGLFramebuffer.h"
#include "../../debug/assert.h"
#include "../debug/log.h"
#include <glad/glad.h>

namespace ge {
namespace renderer {

OpenGLFramebuffer::OpenGLFramebuffer(const FramebufferSpecification &spec)
    : spec_(spec) {
  Invalidate();
}

OpenGLFramebuffer::~OpenGLFramebuffer() {
  glDeleteFramebuffers(1, &rendererID_);
  glDeleteTextures(1, &colorAttachment_);
  glDeleteTextures(1, &entityAttachment_);
  glDeleteTextures(1, &depthAttachment_);
}

void OpenGLFramebuffer::Invalidate() {
  if (rendererID_) {
    glDeleteFramebuffers(1, &rendererID_);
    glDeleteTextures(1, &colorAttachment_);
    glDeleteTextures(1, &entityAttachment_);
    glDeleteTextures(1, &depthAttachment_);
  }

  glCreateFramebuffers(1, &rendererID_);
  glBindFramebuffer(GL_FRAMEBUFFER, rendererID_);

  // Color Attachment
  glCreateTextures(GL_TEXTURE_2D, 1, &colorAttachment_);
  glBindTexture(GL_TEXTURE_2D, colorAttachment_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, spec_.Width, spec_.Height, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         colorAttachment_, 0);

  // Entity ID Attachment
  glCreateTextures(GL_TEXTURE_2D, 1, &entityAttachment_);
  glBindTexture(GL_TEXTURE_2D, entityAttachment_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, spec_.Width, spec_.Height, 0,
               GL_RED_INTEGER, GL_INT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                         entityAttachment_, 0);

  // Depth Attachment
  glCreateTextures(GL_TEXTURE_2D, 1, &depthAttachment_);
  glBindTexture(GL_TEXTURE_2D, depthAttachment_);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH24_STENCIL8, spec_.Width,
                 spec_.Height);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                         GL_TEXTURE_2D, depthAttachment_, 0);

  // Map drawing explicitly to both attachments
  static const GLenum buffers[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
  glDrawBuffers(2, buffers);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    GE_LOG_ERROR("Framebuffer is incomplete!");

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

int OpenGLFramebuffer::ReadPixel(uint32_t attachmentIndex, int x, int y) {
  GE_ASSERT(attachmentIndex < 2, "Invalid attachment index!");
  glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex);
  int pixelData = -1;
  glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &pixelData);
  return pixelData;
}

void OpenGLFramebuffer::Bind() {
  glBindFramebuffer(GL_FRAMEBUFFER, rendererID_);
  glViewport(0, 0, spec_.Width, spec_.Height);
}

void OpenGLFramebuffer::Unbind() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

void OpenGLFramebuffer::Resize(uint32_t width, uint32_t height) {
  spec_.Width = width;
  spec_.Height = height;
  Invalidate();
}

} // namespace renderer
} // namespace ge
