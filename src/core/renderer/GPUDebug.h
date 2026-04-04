#pragma once

#include <string>
#include <cstdint>

namespace ge {
namespace renderer {

#ifndef NDEBUG
    #define GE_GPU_DEBUG 1
#else
    #define GE_GPU_DEBUG 0
#endif

class GPUDebug {
public:
    enum class ObjectType {
        Shader,
        Mesh,
        Texture,
        Framebuffer,
        Buffer,
        RenderPass,
        Pipeline,
        DescriptorSet,
        CommandBuffer
    };

    static void SetEnabled(bool enabled) { s_Enabled = enabled; }
    static bool IsEnabled() { return s_Enabled; }

#if GE_GPU_DEBUG
    static void SetObjectName(ObjectType type, uint64_t handle, const std::string& name);
    static void BeginRegion(uint32_t queueFamily, uint64_t commandBuffer, const std::string& name);
    static void EndRegion(uint32_t queueFamily, uint64_t commandBuffer);
    static void InsertLabel(uint32_t queueFamily, uint64_t commandBuffer, const std::string& name);
#else
    static inline void SetObjectName(ObjectType, uint64_t, const std::string&) {}
    static inline void BeginRegion(uint32_t, uint64_t, const std::string&) {}
    static inline void EndRegion(uint32_t, uint64_t) {}
    static inline void InsertLabel(uint32_t, uint64_t, const std::string&) {}
#endif

private:
    static bool s_Enabled;
};

#if GE_GPU_DEBUG
    #define GE_GPU_NAME(type, handle, name) ::ge::renderer::GPUDebug::SetObjectName(GPUDebug::ObjectType::type, handle, name)
    #define GE_GPU_BEGIN_REGION(queue, cmdBuffer, name) ::ge::renderer::GPUDebug::BeginRegion(queue, cmdBuffer, name)
    #define GE_GPU_END_REGION(queue, cmdBuffer) ::ge::renderer::GPUDebug::EndRegion(queue, cmdBuffer)
    #define GE_GPU_LABEL(queue, cmdBuffer, name) ::ge::renderer::GPUDebug::InsertLabel(queue, cmdBuffer, name)
#else
    #define GE_GPU_NAME(type, handle, name) ((void)0)
    #define GE_GPU_BEGIN_REGION(queue, cmdBuffer, name) ((void)0)
    #define GE_GPU_END_REGION(queue, cmdBuffer) ((void)0)
    #define GE_GPU_LABEL(queue, cmdBuffer, name) ((void)0)
#endif

} // namespace renderer
} // namespace ge
