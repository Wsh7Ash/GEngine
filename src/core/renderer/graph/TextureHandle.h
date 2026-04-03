#pragma once

// ================================================================
//  TextureHandle.h
//  Smart handle for transient textures in render graph.
// ================================================================

#include "../Framebuffer.h"
#include <string>
#include <memory>
#include <functional>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace ge {
namespace renderer {

enum class TextureUsage {
    ColorAttachment,
    DepthAttachment,
    ShaderRead,
    ShaderWrite,
    RenderTargetRead,
    RenderTargetWrite,
    DepthStencil,
    ResolveSource,
    ResolveDest
};

enum class TextureState {
    Undefined,
    Initialized,
    ColorAttachment,
    DepthStencilAttachment,
    ShaderReadOnly,
    ShaderReadWrite,
    RenderTarget,
    CopySource,
    CopyDest,
    ResolveSource,
    ResolveDest
};

inline TextureState CombineStates(TextureState a, TextureState b) {
    if (a == b) return a;
    if (a == TextureState::ShaderReadOnly && b == TextureState::ShaderReadWrite) {
        return TextureState::ShaderReadWrite;
    }
    if (b == TextureState::ShaderReadOnly && a == TextureState::ShaderReadWrite) {
        return TextureState::ShaderReadWrite;
    }
    return a;
}

struct TextureDesc {
    uint32_t width = 0;
    uint32_t height = 0;
    FramebufferTextureFormat format = FramebufferTextureFormat::RGBA8;
    uint32_t samples = 1;
    bool createMipmaps = false;
    std::string debugName;
    
    TextureDesc() = default;
    TextureDesc(uint32_t w, uint32_t h, FramebufferTextureFormat f = FramebufferTextureFormat::RGBA8)
        : width(w), height(h), format(f) {}
};

class TextureHandle {
public:
    TextureHandle();
    explicit TextureHandle(const std::string& name);
    TextureHandle(const std::string& name, const TextureDesc& desc);
    ~TextureHandle();
    
    TextureHandle(const TextureHandle& other);
    TextureHandle& operator=(const TextureHandle& other);
    TextureHandle(TextureHandle&& other) noexcept;
    TextureHandle& operator=(TextureHandle&& other) noexcept;
    
    void Reset();
    
    bool IsValid() const { return framebuffer_ != nullptr; }
    
    const std::string& GetName() const { return name_; }
    const TextureDesc& GetDesc() const { return desc_; }
    
    std::shared_ptr<Framebuffer> GetFramebuffer() const { return framebuffer_; }
    uint32_t GetRendererID() const { return framebuffer_ ? framebuffer_->GetColorAttachmentRendererID() : 0; }
    uint32_t GetDepthRendererID() const { return framebuffer_ ? framebuffer_->GetDepthAttachmentRendererID() : 0; }
    
    void SetState(TextureState state) { currentState_ = state; }
    TextureState GetState() const { return currentState_; }
    
    void SetUsage(TextureUsage usage) { usage_ = usage; }
    TextureUsage GetUsage() const { return usage_; }
    
    void AddRef() { refCount_++; }
    void ReleaseRef() { if (refCount_ > 0) refCount_--; }
    int GetRefCount() const { return refCount_; }
    
    void Resize(uint32_t width, uint32_t height);
    bool NeedsResize() const;
    
    void SetDebugName(const std::string& name) { debugName_ = name; }
    const std::string& GetDebugName() const { return debugName_; }
    
    bool operator==(const TextureHandle& other) const { return name_ == other.name_; }
    bool operator!=(const TextureHandle& other) const { return !(*this == other); }
    
private:
    void CreateFramebuffer();
    
    std::string name_;
    TextureDesc desc_;
    TextureState currentState_ = TextureState::Undefined;
    TextureUsage usage_ = TextureUsage::ShaderRead;
    std::shared_ptr<Framebuffer> framebuffer_;
    int refCount_ = 0;
    std::string debugName_;
};

class RenderGraph;
class TexturePool {
public:
    TexturePool();
    ~TexturePool();
    
    TextureHandle Allocate(const std::string& name, const TextureDesc& desc);
    void Release(const std::string& name);
    void Release(TextureHandle& handle);
    
    void Clear();
    
    size_t GetAllocatedCount() const { return allocated_.size(); }
    size_t GetTotalMemoryUsage() const { return totalMemory_; }
    
    void SetMaxMemory(size_t maxBytes) { maxMemory_ = maxBytes; }
    size_t GetMaxMemory() const { return maxMemory_; }
    
private:
    struct PoolEntry {
        TextureHandle handle;
        bool inUse;
    };
    
    std::unordered_map<std::string, PoolEntry> allocated_;
    size_t totalMemory_ = 0;
    size_t maxMemory_ = 256 * 1024 * 1024; // 256MB default
};

struct PassInput {
    std::string name;
    TextureHandle texture;
};

struct PassOutput {
    std::string name;
    TextureHandle texture;
    TextureState expectedState;
};

class RenderGraph;

} // namespace renderer
} // namespace ge
