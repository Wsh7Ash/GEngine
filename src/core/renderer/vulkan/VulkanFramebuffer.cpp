#include "VulkanFramebuffer.h"
#include "VulkanContext.h"
#include "../../debug/log.h"

#include <algorithm>

namespace ge {
namespace renderer {

VulkanFramebuffer::VulkanFramebuffer(const FramebufferSpecification& spec)
    : spec_(spec)
{
    Invalidate();
}

VulkanFramebuffer::~VulkanFramebuffer()
{
    Destroy();
}

void VulkanFramebuffer::Invalidate()
{
    VkDevice device = VulkanContext::Get().GetDevice();

    Destroy();

    isMultisampled_ = spec_.Samples > 1;

    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkAttachmentReference> colorAttachmentRefs;
    VkAttachmentReference depthAttachmentRef = {};

    uint32_t colorAttachmentCount = GetAttachmentCount();
    colorAttachmentRefs.resize(colorAttachmentCount);

    for (uint32_t i = 0; i < colorAttachmentCount; i++) {
        auto& spec = spec_.Attachments.Attachments[i];
        VkFormat format = GetVkFormat(spec.TextureFormat);

        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format = format;
        colorAttachment.samples = isMultisampled_ ? static_cast<VkSampleCountFlagBits>(spec_.Samples) : VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        attachments.push_back(colorAttachment);

        colorAttachmentRefs[i].attachment = i;
        colorAttachmentRefs[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    bool hasDepth = spec_.Attachments.Attachments.empty() == false && 
        spec_.Attachments.Attachments[0].TextureFormat == FramebufferTextureFormat::DEPTH24STENCIL8;

    if (hasDepth) {
        VkAttachmentDescription depthAttachment = {};
        depthAttachment.format = GetVkFormat(FramebufferTextureFormat::DEPTH24STENCIL8);
        depthAttachment.samples = isMultisampled_ ? static_cast<VkSampleCountFlagBits>(spec_.Samples) : VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        attachments.push_back(depthAttachment);

        depthAttachmentRef.attachment = colorAttachmentCount;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentRefs.size());
    subpass.pColorAttachments = colorAttachmentRefs.data();
    subpass.pDepthStencilAttachment = hasDepth ? &depthAttachmentRef : nullptr;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    VkResult result = vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass_);
    if (result != VK_SUCCESS) {
        GE_LOG_ERROR("VulkanFramebuffer: Failed to create render pass");
        return;
    }

    std::vector<VkImageView> attachmentViews;

    for (uint32_t i = 0; i < colorAttachmentCount; i++) {
        AttachmentInfo attachment;
        attachment.spec = spec_.Attachments.Attachments[i];

        VkImageCreateInfo imageInfo = {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = spec_.Width;
        imageInfo.extent.height = spec_.Height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = GetVkFormat(attachment.spec.TextureFormat);
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = isMultisampled_ ? static_cast<VkSampleCountFlagBits>(spec_.Samples) : VK_SAMPLE_COUNT_1_BIT;

        result = vkCreateImage(device, &imageInfo, nullptr, &attachment.image);
        if (result != VK_SUCCESS) {
            GE_LOG_ERROR("VulkanFramebuffer: Failed to create color image");
            continue;
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, attachment.image, &memRequirements);

        VkMemoryAllocateInfo memAllocInfo = {};
        memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memAllocInfo.allocationSize = memRequirements.size;
        memAllocInfo.memoryTypeIndex = 0;

        uint32_t memoryTypeBits = memRequirements.memoryTypeBits;
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(VulkanContext::Get().GetPhysicalDevice(), &memProperties);

        for (uint32_t j = 0; j < memProperties.memoryTypeCount; j++) {
            if ((memoryTypeBits & (1 << j)) && (memProperties.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
                memAllocInfo.memoryTypeIndex = j;
                break;
            }
        }

        result = vkAllocateMemory(device, &memAllocInfo, nullptr, &attachment.memory);
        if (result != VK_SUCCESS) {
            GE_LOG_ERROR("VulkanFramebuffer: Failed to allocate color image memory");
            continue;
        }

        result = vkBindImageMemory(device, attachment.image, attachment.memory, 0);
        if (result != VK_SUCCESS) {
            GE_LOG_ERROR("VulkanFramebuffer: Failed to bind color image memory");
            continue;
        }

        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = attachment.image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = GetVkFormat(attachment.spec.TextureFormat);
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        result = vkCreateImageView(device, &viewInfo, nullptr, &attachment.imageView);
        if (result != VK_SUCCESS) {
            GE_LOG_ERROR("VulkanFramebuffer: Failed to create color image view");
            continue;
        }

        attachment.rendererID = static_cast<uint32_t>(reinterpret_cast<uint64_t>(attachment.imageView));
        colorAttachments_.push_back(attachment);
        attachmentViews.push_back(attachment.imageView);
    }

    if (hasDepth) {
        AttachmentInfo depthAttachment;
        depthAttachment.spec = spec_.Attachments.Attachments[0];

        VkImageCreateInfo imageInfo = {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = spec_.Width;
        imageInfo.extent.height = spec_.Height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = GetVkFormat(FramebufferTextureFormat::DEPTH24STENCIL8);
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = isMultisampled_ ? static_cast<VkSampleCountFlagBits>(spec_.Samples) : VK_SAMPLE_COUNT_1_BIT;

        result = vkCreateImage(device, &imageInfo, nullptr, &depthAttachment.image);
        if (result != VK_SUCCESS) {
            GE_LOG_ERROR("VulkanFramebuffer: Failed to create depth image");
        } else {
            VkMemoryRequirements memRequirements;
            vkGetImageMemoryRequirements(device, depthAttachment.image, &memRequirements);

            VkMemoryAllocateInfo memAllocInfo = {};
            memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            memAllocInfo.allocationSize = memRequirements.size;
            memAllocInfo.memoryTypeIndex = 0;

            uint32_t memoryTypeBits = memRequirements.memoryTypeBits;
            VkPhysicalDeviceMemoryProperties memProperties;
            vkGetPhysicalDeviceMemoryProperties(VulkanContext::Get().GetPhysicalDevice(), &memProperties);

            for (uint32_t j = 0; j < memProperties.memoryTypeCount; j++) {
                if ((memoryTypeBits & (1 << j)) && (memProperties.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
                    memAllocInfo.memoryTypeIndex = j;
                    break;
                }
            }

            result = vkAllocateMemory(device, &memAllocInfo, nullptr, &depthAttachment.memory);
            if (result == VK_SUCCESS) {
                vkBindImageMemory(device, depthAttachment.image, depthAttachment.memory, 0);
            }

            VkImageViewCreateInfo viewInfo = {};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = depthAttachment.image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = GetVkFormat(FramebufferTextureFormat::DEPTH24STENCIL8);
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            vkCreateImageView(device, &viewInfo, nullptr, &depthAttachment.imageView);

            depthAttachment.rendererID = static_cast<uint32_t>(reinterpret_cast<uint64_t>(depthAttachment.imageView));
            depthAttachment_ = depthAttachment;
            attachmentViews.push_back(depthAttachment.imageView);
        }
    }

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass_;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachmentViews.size());
    framebufferInfo.pAttachments = attachmentViews.data();
    framebufferInfo.width = spec_.Width;
    framebufferInfo.height = spec_.Height;
    framebufferInfo.layers = 1;

    result = vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffer_);
    if (result != VK_SUCCESS) {
        GE_LOG_ERROR("VulkanFramebuffer: Failed to create framebuffer");
    }

    GE_LOG_INFO("VulkanFramebuffer: Created {}x{} with {} color attachments", 
        spec_.Width, spec_.Height, colorAttachmentCount);
}

void VulkanFramebuffer::Destroy()
{
    VkDevice device = VulkanContext::Get().GetDevice();

    if (framebuffer_ != VK_NULL_HANDLE) {
        vkDestroyFramebuffer(device, framebuffer_, nullptr);
        framebuffer_ = VK_NULL_HANDLE;
    }

    if (renderPass_ != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device, renderPass_, nullptr);
        renderPass_ = VK_NULL_HANDLE;
    }

    for (auto& attachment : colorAttachments_) {
        if (attachment.imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(device, attachment.imageView, nullptr);
        }
        if (attachment.image != VK_NULL_HANDLE) {
            vkDestroyImage(device, attachment.image, nullptr);
        }
        if (attachment.memory != VK_NULL_HANDLE) {
            vkFreeMemory(device, attachment.memory, nullptr);
        }
    }
    colorAttachments_.clear();

    if (depthAttachment_.imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, depthAttachment_.imageView, nullptr);
    }
    if (depthAttachment_.image != VK_NULL_HANDLE) {
        vkDestroyImage(device, depthAttachment_.image, nullptr);
    }
    if (depthAttachment_.memory != VK_NULL_HANDLE) {
        vkFreeMemory(device, depthAttachment_.memory, nullptr);
    }
    depthAttachment_ = {};
}

void VulkanFramebuffer::Bind()
{
    VkCommandBuffer cmdBuffer = VulkanContext::Get().GetCurrentCommandBuffer();

    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = renderPass_;
    renderPassBeginInfo.framebuffer = framebuffer_;
    renderPassBeginInfo.renderArea.offset = { 0, 0 };
    renderPassBeginInfo.renderArea.extent = { spec_.Width, spec_.Height };

    std::vector<VkClearValue> clearValues;
    for (size_t i = 0; i < colorAttachments_.size(); i++) {
        VkClearValue clearValue = {};
        clearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };
        clearValues.push_back(clearValue);
    }
    if (depthAttachment_.image != VK_NULL_HANDLE) {
        VkClearValue clearValue = {};
        clearValue.depthStencil = { 1.0f, 0 };
        clearValues.push_back(clearValue);
    }

    renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassBeginInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanFramebuffer::Unbind()
{
    VkCommandBuffer cmdBuffer = VulkanContext::Get().GetCurrentCommandBuffer();
    vkCmdEndRenderPass(cmdBuffer);
}

void VulkanFramebuffer::Resize(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0) return;

    spec_.Width = width;
    spec_.Height = height;
    Invalidate();
}

uint32_t VulkanFramebuffer::GetColorAttachmentRendererID(uint32_t index) const
{
    if (index < colorAttachments_.size()) {
        return colorAttachments_[index].rendererID;
    }
    return 0;
}

uint32_t VulkanFramebuffer::GetDepthAttachmentRendererID() const
{
    return depthAttachment_.rendererID;
}

uint32_t VulkanFramebuffer::GetEntityAttachmentRendererID() const
{
    if (colorAttachments_.size() > 1) {
        return colorAttachments_[1].rendererID;
    }
    return 0;
}

void VulkanFramebuffer::ClearAttachment(uint32_t attachmentIndex, int value)
{
    VkCommandBuffer cmdBuffer = VulkanContext::Get().GetCurrentCommandBuffer();

    if (attachmentIndex < colorAttachments_.size()) {
        VkClearAttachment clearAttachment = {};
        clearAttachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        clearAttachment.colorAttachment = attachmentIndex;
        clearAttachment.clearValue.color = { 
            static_cast<float>((value >> 0) & 0xFF) / 255.0f,
            static_cast<float>((value >> 8) & 0xFF) / 255.0f,
            static_cast<float>((value >> 16) & 0xFF) / 255.0f,
            static_cast<float>((value >> 24) & 0xFF) / 255.0f
        };

        VkClearRect clearRect = {};
        clearRect.rect.offset = { 0, 0 };
        clearRect.rect.extent = { spec_.Width, spec_.Height };
        clearRect.baseArrayLayer = 0;
        clearRect.layerCount = 1;

        vkCmdClearAttachments(cmdBuffer, 1, &clearAttachment, 1, &clearRect);
    }
}

int VulkanFramebuffer::ReadPixel(uint32_t attachmentIndex, int x, int y)
{
    return 0;
}

VkFormat VulkanFramebuffer::GetVkFormat(FramebufferTextureFormat format) const
{
    switch (format) {
        case FramebufferTextureFormat::RGBA8:      return VK_FORMAT_R8G8B8A8_UNORM;
        case FramebufferTextureFormat::RGBA16F:    return VK_FORMAT_R16G16B16A16_SFLOAT;
        case FramebufferTextureFormat::RG16F:      return VK_FORMAT_R16G16_SFLOAT;
        case FramebufferTextureFormat::RED8:       return VK_FORMAT_R8_UNORM;
        case FramebufferTextureFormat::RED_INTEGER: return VK_FORMAT_R32_SINT;
        case FramebufferTextureFormat::DEPTH24STENCIL8: return VK_FORMAT_D24_UNORM_S8_UINT;
        default:                                    return VK_FORMAT_R8G8B8A8_UNORM;
    }
}

uint32_t VulkanFramebuffer::GetAttachmentCount() const
{
    return static_cast<uint32_t>(spec_.Attachments.Attachments.size());
}

} // namespace renderer
} // namespace ge
