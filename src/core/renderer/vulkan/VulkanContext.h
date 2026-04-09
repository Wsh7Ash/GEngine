#pragma once

// ================================================================
//  VulkanContext.h
//  Vulkan graphics context implementation.
// ================================================================

/**
 * @file VulkanContext.h
 * @brief Vulkan graphics context with pipeline cache support.
 * 
 * Pipeline cache is persisted to disk for faster startup times.
 * Cache file location: %LOCALAPPDATA%/GEngine/pipeline_cache.bin (Windows)
 *                     ~/.local/share/GEngine/pipeline_cache.bin (Linux/macOS)
 */

#include "../GraphicsContext.h"
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <optional>
#include <cstring>
#include <memory>

namespace ge {
namespace renderer {

struct VulkanQueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool IsComplete() const {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct VulkanSwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class VulkanContext : public GraphicsContext {
public:
    VulkanContext(GLFWwindow* window);
    ~VulkanContext() override;

    void Init() override;
    void SwapBuffers() override;

    static VulkanContext& Get();

    VkInstance GetInstance() const { return instance_; }
    VkDevice GetDevice() const { return device_; }
    VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice_; }
    VkSurfaceKHR GetSurface() const { return surface_; }
    VkSwapchainKHR GetSwapchain() const { return swapchain_; }
    VkCommandPool GetCommandPool() const { return commandPool_; }
    VkCommandBuffer GetCurrentCommandBuffer() const { return commandBuffers_[currentFrame_]; }
    VkQueue GetGraphicsQueue() const { return graphicsQueue_; }
    VkQueue GetPresentQueue() const { return presentQueue_; }
    VkRenderPass GetRenderPass() const { return renderPass_; }

    const VulkanSwapChainSupportDetails& GetSwapChainSupport() const { return swapChainSupport_; }
    const VulkanQueueFamilyIndices& GetQueueFamilies() const { return queueFamilies_; }
    uint32_t GetCurrentFrame() const { return currentFrame_; }
    uint32_t GetImageCount() const { return static_cast<uint32_t>(swapchainImages_.size()); }

    bool HasValidationLayers() const { return enableValidationLayers_; }
    const std::vector<const char*>& GetExtensions() const { return requiredExtensions_; }

    void CreateRenderPass();
    void CreateCommandBuffers();
    void RecreateSwapchain();
    void OnWindowResize(int width, int height);

    void CreateSyncPrimitives();
    void CreateSwapchainFramebuffers();

    VkExtent2D GetSwapExtent() const { return swapExtent_; }
    VkPipelineCache GetPipelineCache() const { return pipelineCache_; }

    static std::string GetPipelineCachePath();

private:
    bool LoadPipelineCache();
    bool SavePipelineCache();
    void CreateInstance();
    void SetupDebugMessenger();
    void CreateSurface();
    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateSwapchain();
    void CreateRenderPassInternal();
    void CreateCommandPool();

    bool CheckValidationLayerSupport();
    bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
    std::vector<const char*> GetRequiredExtensions();
    VulkanQueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
    VulkanSwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);

    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    static VKAPI_PTR VkDebugUtilsMessengerEXT callback_;
    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator);
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks* pAllocator);

    GLFWwindow* window_ = nullptr;

    VkInstance instance_ = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;

    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;

    VkQueue graphicsQueue_ = VK_NULL_HANDLE;
    VkQueue presentQueue_ = VK_NULL_HANDLE;

    VulkanSwapChainSupportDetails swapChainSupport_;
    VulkanQueueFamilyIndices queueFamilies_;

    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    std::vector<VkImage> swapchainImages_;
    std::vector<VkImageView> swapchainImageViews_;
    std::vector<VkFramebuffer> swapchainFramebuffers_;
    VkExtent2D swapExtent_ = {0, 0};

    VkSemaphore imageAvailableSemaphore_ = VK_NULL_HANDLE;
    VkSemaphore renderCompleteSemaphore_ = VK_NULL_HANDLE;
    VkFence inFlightFence_ = VK_NULL_HANDLE;

    VkPipelineCache pipelineCache_ = VK_NULL_HANDLE;

    VkRenderPass renderPass_ = VK_NULL_HANDLE;
    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers_;

    uint32_t currentFrame_ = 0;
    uint32_t imageCount_ = 2;

    bool enableValidationLayers_ = true;
    std::vector<const char*> requiredExtensions_;
    std::vector<const char*> deviceExtensions_ = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    const std::vector<const char*> validationLayers_ = {
        "VK_LAYER_KHRONOS_validation"
    };
};

} // namespace renderer
} // namespace ge
