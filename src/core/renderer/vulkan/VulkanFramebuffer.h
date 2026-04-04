#pragma once

#include "../Framebuffer.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

namespace ge {
namespace renderer {

class VulkanFramebuffer : public Framebuffer {
public:
    VulkanFramebuffer(const FramebufferSpecification& spec);
    virtual ~VulkanFramebuffer() override;

    virtual void Bind() override;
    virtual void Unbind() override;
    virtual void Resize(uint32_t width, uint32_t height) override;
    virtual uint32_t GetColorAttachmentRendererID(uint32_t index = 0) const override;
    virtual uint32_t GetDepthAttachmentRendererID() const override;
    virtual uint32_t GetEntityAttachmentRendererID() const override;
    virtual void ClearAttachment(uint32_t attachmentIndex, int value) override;
    virtual int ReadPixel(uint32_t attachmentIndex, int x, int y) override;
    virtual const FramebufferSpecification& GetSpecification() const override { return spec_; }

    VkRenderPass GetRenderPass() const { return renderPass_; }
    VkFramebuffer GetFramebuffer() const { return framebuffer_; }

private:
    struct AttachmentInfo {
        VkImage image = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        uint32_t rendererID = 0;
        FramebufferTextureSpecification spec;
    };

    void Invalidate();
    void Destroy();
    VkFormat GetVkFormat(FramebufferTextureFormat format) const;
    uint32_t GetAttachmentCount() const;

    FramebufferSpecification spec_;
    std::vector<AttachmentInfo> colorAttachments_;
    AttachmentInfo depthAttachment_;
    VkRenderPass renderPass_ = VK_NULL_HANDLE;
    VkFramebuffer framebuffer_ = VK_NULL_HANDLE;

    bool isMultisampled_ = false;
};

} // namespace renderer
} // namespace ge
