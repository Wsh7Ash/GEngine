#pragma once

// ================================================================
//  VulkanShader.h
//  Vulkan shader implementation.
// ================================================================

#include "../Shader.h"
#include <vulkan/vulkan.h>
#include <string>
#include <unordered_map>

namespace ge {
namespace renderer {

class VulkanShader : public Shader {
public:
    VulkanShader(const std::string& filepath);
    VulkanShader(const std::string& vertexPath, const std::string& fragmentPath);
    ~VulkanShader() override;

    void Bind() const override;
    void Unbind() const override;

    void SetInt(const std::string& name, int value) override;
    void SetBool(const std::string& name, bool value) override;
    void SetFloat(const std::string& name, float value) override;
    void SetVec3(const std::string& name, const Math::Vec3f& value) override;
    void SetVec4(const std::string& name, const Math::Vec4f& value) override;
    void SetMat4(const std::string& name, const Math::Mat4f& value) override;
    void SetMat4Array(const std::string& name, const Math::Mat4f* values, uint32_t count) override;

    virtual bool Reload() override;

    virtual void UseVariant(const std::string& key) override;

    VkPipeline GetPipeline() const { return pipeline_; }
    VkPipelineLayout GetPipelineLayout() const { return pipelineLayout_; }
    VkDescriptorSetLayout GetDescriptorSetLayout() const { return descriptorSetLayout_; }

private:
    void CompileOrGetVulkanShader(const std::string& name, const std::vector<uint32_t>& spv);
    void CreatePipeline();

    VkShaderModule vertexShader_ = VK_NULL_HANDLE;
    VkShaderModule fragmentShader_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout_ = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet_ = VK_NULL_HANDLE;

    std::string vertexFilepath_;
    std::string fragmentFilepath_;
    std::string shaderName_;

    std::unordered_map<std::string, int> uniformLocationCache_;
    std::unordered_map<std::string, VkPipeline> variantPipelines_;
};

} // namespace renderer
} // namespace ge
