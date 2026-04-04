#include "VulkanMesh.h"
#include "VulkanContext.h"
#include "../../debug/log.h"
#include "../../debug/assert.h"

#include <cstring>
#include <algorithm>

namespace ge {
namespace renderer {

VulkanMesh::VulkanMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
    : vertices_(vertices), indices_(indices), vertexCount_(static_cast<uint32_t>(vertices.size())),
      indexCount_(static_cast<uint32_t>(indices.size())), maxVertices_(vertexCount_), maxIndices_(indexCount_)
{
    CreateBuffers(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    float minX = FLT_MAX, minY = FLT_MAX, minZ = FLT_MAX;
    float maxX = -FLT_MAX, maxY = -FLT_MAX, maxZ = -FLT_MAX;
    for (const auto& v : vertices_) {
        minX = std::min(minX, v.Position[0]);
        minY = std::min(minY, v.Position[1]);
        minZ = std::min(minZ, v.Position[2]);
        maxX = std::max(maxX, v.Position[0]);
        maxY = std::max(maxY, v.Position[1]);
        maxZ = std::max(maxZ, v.Position[2]);
    }
    aabb_ = Math::AABB(glm::vec3(minX, minY, minZ), glm::vec3(maxX, maxY, maxZ));

    GE_LOG_INFO("VulkanMesh: Created with {0} vertices, {1} indices", vertexCount_, indexCount_);
}

VulkanMesh::VulkanMesh(uint32_t maxVertices, uint32_t maxIndices)
    : maxVertices_(maxVertices), maxIndices_(maxIndices)
{
    vertices_.resize(maxVertices);
    indices_.resize(maxIndices);

    CreateBuffers(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    aabb_ = Math::AABB(glm::vec3(0), glm::vec3(0));
}

VulkanMesh::~VulkanMesh()
{
    VkDevice device = VulkanContext::Get().GetDevice();

    if (vertexBuffer_) {
        vkDestroyBuffer(device, vertexBuffer_, nullptr);
    }
    if (vertexMemory_) {
        vkFreeMemory(device, vertexMemory_, nullptr);
    }
    if (indexBuffer_) {
        vkDestroyBuffer(device, indexBuffer_, nullptr);
    }
    if (indexMemory_) {
        vkFreeMemory(device, indexMemory_, nullptr);
    }
}

void VulkanMesh::CreateBuffers(VkBufferUsageFlags usage)
{
    VkDevice device = VulkanContext::Get().GetDevice();
    VkPhysicalDevice physicalDevice = VulkanContext::Get().GetPhysicalDevice();

    VkDeviceSize vertexBufferSize = maxVertices_ * sizeof(Vertex);
    VkDeviceSize indexBufferSize = maxIndices_ * sizeof(uint32_t);

    VkBufferCreateInfo vertexBufferInfo = {};
    vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertexBufferInfo.size = vertexBufferSize;
    vertexBufferInfo.usage = usage;
    vertexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(device, &vertexBufferInfo, nullptr, &vertexBuffer_);
    if (result != VK_SUCCESS) {
        GE_LOG_ERROR("VulkanMesh: Failed to create vertex buffer");
        return;
    }

    VkMemoryRequirements vertexMemRequirements;
    vkGetBufferMemoryRequirements(device, vertexBuffer_, &vertexMemRequirements);

    VkMemoryAllocateInfo vertexMemAllocInfo = {};
    vertexMemAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    vertexMemAllocInfo.allocationSize = vertexMemRequirements.size;
    vertexMemAllocInfo.memoryTypeIndex = 0;

    uint32_t memoryTypeBits = vertexMemRequirements.memoryTypeBits;
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((memoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
            vertexMemAllocInfo.memoryTypeIndex = i;
            break;
        }
    }

    result = vkAllocateMemory(device, &vertexMemAllocInfo, nullptr, &vertexMemory_);
    if (result != VK_SUCCESS) {
        GE_LOG_ERROR("VulkanMesh: Failed to allocate vertex memory");
        return;
    }

    result = vkBindBufferMemory(device, vertexBuffer_, vertexMemory_, 0);
    if (result != VK_SUCCESS) {
        GE_LOG_ERROR("VulkanMesh: Failed to bind vertex buffer memory");
        return;
    }

    if (!vertices_.empty()) {
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingMemory;
        CreateStagingBuffer(stagingBuffer, stagingMemory, vertexBufferSize, vertices_.data());
        CopyBuffer(stagingBuffer, vertexBuffer_, vertexBufferSize);
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingMemory, nullptr);
    }

    VkBufferCreateInfo indexBufferInfo = {};
    indexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    indexBufferInfo.size = indexBufferSize;
    indexBufferInfo.usage = usage;
    indexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    result = vkCreateBuffer(device, &indexBufferInfo, nullptr, &indexBuffer_);
    if (result != VK_SUCCESS) {
        GE_LOG_ERROR("VulkanMesh: Failed to create index buffer");
        return;
    }

    VkMemoryRequirements indexMemRequirements;
    vkGetBufferMemoryRequirements(device, indexBuffer_, &indexMemRequirements);

    VkMemoryAllocateInfo indexMemAllocInfo = {};
    indexMemAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    indexMemAllocInfo.allocationSize = indexMemRequirements.size;
    indexMemAllocInfo.memoryTypeIndex = 0;

    memoryTypeBits = indexMemRequirements.memoryTypeBits;
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((memoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
            indexMemAllocInfo.memoryTypeIndex = i;
            break;
        }
    }

    result = vkAllocateMemory(device, &indexMemAllocInfo, nullptr, &indexMemory_);
    if (result != VK_SUCCESS) {
        GE_LOG_ERROR("VulkanMesh: Failed to allocate index memory");
        return;
    }

    result = vkBindBufferMemory(device, indexBuffer_, indexMemory_, 0);
    if (result != VK_SUCCESS) {
        GE_LOG_ERROR("VulkanMesh: Failed to bind index buffer memory");
        return;
    }

    if (!indices_.empty()) {
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingMemory;
        CreateStagingBuffer(stagingBuffer, stagingMemory, indexBufferSize, indices_.data());
        CopyBuffer(stagingBuffer, indexBuffer_, indexBufferSize);
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingMemory, nullptr);
    }
}

void VulkanMesh::CreateStagingBuffer(VkBuffer& stagingBuffer, VkDeviceMemory& stagingMemory, VkDeviceSize size, const void* data)
{
    VkDevice device = VulkanContext::Get().GetDevice();
    VkPhysicalDevice physicalDevice = VulkanContext::Get().GetPhysicalDevice();

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateBuffer(device, &bufferInfo, nullptr, &stagingBuffer);

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, stagingBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = 0;

    uint32_t memoryTypeBits = memRequirements.memoryTypeBits;
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((memoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
            allocInfo.memoryTypeIndex = i;
            break;
        }
    }

    vkAllocateMemory(device, &allocInfo, nullptr, &stagingMemory);
    vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0);

    void* mappedMemory;
    vkMapMemory(device, stagingMemory, 0, size, 0, &mappedMemory);
    memcpy(mappedMemory, data, static_cast<size_t>(size));
    vkUnmapMemory(device, stagingMemory);
}

void VulkanMesh::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkDevice device = VulkanContext::Get().GetDevice();
    VkCommandPool commandPool = VulkanContext::Get().GetCommandPool();
    VkQueue graphicsQueue = VulkanContext::Get().GetGraphicsQueue();

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion = {};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void VulkanMesh::Bind() const
{
}

void VulkanMesh::Unbind() const
{
}

void VulkanMesh::Draw() const
{
    VkCommandBuffer cmdBuffer = VulkanContext::Get().GetCurrentCommandBuffer();
    VkDevice device = VulkanContext::Get().GetDevice();

    VkBuffer vertexBuffers[] = { vertexBuffer_ };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(cmdBuffer, indexBuffer_, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cmdBuffer, indexCount_, 1, 0, 0, 0);
}

void VulkanMesh::DrawInstanced(const std::vector<Math::Mat4f>& instances)
{
    VkCommandBuffer cmdBuffer = VulkanContext::Get().GetCurrentCommandBuffer();

    VkBuffer vertexBuffers[] = { vertexBuffer_ };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(cmdBuffer, indexBuffer_, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cmdBuffer, indexCount_, static_cast<uint32_t>(instances.size()), 0, 0, 0);
}

void VulkanMesh::SetData(const void* vertices, uint32_t size)
{
    if (!vertexBuffer_) return;

    VkDevice device = VulkanContext::Get().GetDevice();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    CreateStagingBuffer(stagingBuffer, stagingMemory, size, vertices);
    CopyBuffer(stagingBuffer, vertexBuffer_, size);
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);

    vertexCount_ = size / sizeof(Vertex);
}

void VulkanMesh::SetIndices(const uint32_t* indices, uint32_t count)
{
    if (!indexBuffer_) return;

    VkDevice device = VulkanContext::Get().GetDevice();

    VkDeviceSize size = count * sizeof(uint32_t);
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    CreateStagingBuffer(stagingBuffer, stagingMemory, size, indices);
    CopyBuffer(stagingBuffer, indexBuffer_, size);
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);

    indexCount_ = count;
}

} // namespace renderer
} // namespace ge
