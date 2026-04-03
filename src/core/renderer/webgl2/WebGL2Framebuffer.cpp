#include "WebGL2Framebuffer.h"
#include "../../debug/log.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace ge {
namespace renderer {

WebGL2Framebuffer::WebGL2Framebuffer(const FramebufferSpecification& spec)
    : specification_(spec)
{
    Invalidate();
}

WebGL2Framebuffer::~WebGL2Framebuffer()
{
    Destroy();
}

void WebGL2Framebuffer::Invalidate()
{
    Destroy();

    glGenFramebuffers(1, &rendererID_);
    glBindFramebuffer(GL_FRAMEBUFFER, rendererID_);

    if (!specification_.SwapChainTarget) {
        if (!specification_.Attachments.Attachments.empty()) {
            for (size_t i = 0; i < specification_.Attachments.Attachments.size(); ++i) {
                auto& attachment = specification_.Attachments.Attachments[i];
                switch (attachment.TextureFormat) {
                    case FramebufferTextureFormat::RGBA8: {
                        glGenTextures(1, &colorAttachment_);
                        glBindTexture(GL_TEXTURE_2D, colorAttachment_);
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, specification_.Width, specification_.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i), GL_TEXTURE_2D, colorAttachment_, 0);
                        break;
                    }
                    case FramebufferTextureFormat::RGBA16F: {
                        glGenTextures(1, &colorAttachment_);
                        glBindTexture(GL_TEXTURE_2D, colorAttachment_);
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, specification_.Width, specification_.Height, 0, GL_RGBA, GL_FLOAT, nullptr);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i), GL_TEXTURE_2D, colorAttachment_, 0);
                        break;
                    }
                    case FramebufferTextureFormat::DEPTH24STENCIL8: {
                        glGenTextures(1, &depthAttachment_);
                        glBindTexture(GL_TEXTURE_2D, depthAttachment_);
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, specification_.Width, specification_.Height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthAttachment_, 0);
                        break;
                    }
                    default:
                        break;
                }
            }
        }
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        GE_LOG_ERROR("Framebuffer is not complete!");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void WebGL2Framebuffer::Destroy()
{
    if (rendererID_) {
        glDeleteFramebuffers(1, &rendererID_);
        rendererID_ = 0;
    }
    if (colorAttachment_) {
        glDeleteTextures(1, &colorAttachment_);
        colorAttachment_ = 0;
    }
    if (depthAttachment_) {
        glDeleteTextures(1, &depthAttachment_);
        depthAttachment_ = 0;
    }
    if (entityAttachment_) {
        glDeleteTextures(1, &entityAttachment_);
        entityAttachment_ = 0;
    }
}

void WebGL2Framebuffer::Bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, rendererID_);
    glViewport(0, 0, specification_.Width, specification_.Height);
}

void WebGL2Framebuffer::Unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void WebGL2Framebuffer::Resize(uint32_t width, uint32_t height)
{
    specification_.Width = width;
    specification_.Height = height;
    Invalidate();
}

uint32_t WebGL2Framebuffer::GetColorAttachmentRendererID(uint32_t index) const
{
    (void)index;
    return colorAttachment_;
}

uint32_t WebGL2Framebuffer::GetDepthAttachmentRendererID() const
{
    return depthAttachment_;
}

uint32_t WebGL2Framebuffer::GetEntityAttachmentRendererID() const
{
    return entityAttachment_;
}

void WebGL2Framebuffer::ClearAttachment(uint32_t attachmentIndex, int value)
{
    (void)attachmentIndex;
    (void)value;
}

int WebGL2Framebuffer::ReadPixel(uint32_t attachmentIndex, int x, int y)
{
    (void)attachmentIndex;
    (void)x;
    (void)y;
    return 0;
}

} // namespace renderer
} // namespace ge
