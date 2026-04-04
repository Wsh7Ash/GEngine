#pragma once

#include "../Framebuffer.h"
#include <d3d11.h>
#include <vector>

namespace ge {
namespace renderer {

class DX11Framebuffer : public Framebuffer {
public:
    DX11Framebuffer(const FramebufferSpecification& spec);
    virtual ~DX11Framebuffer() override;

    virtual void Bind() override;
    virtual void Unbind() override;
    virtual void Resize(uint32_t width, uint32_t height) override;
    virtual uint32_t GetColorAttachmentRendererID(uint32_t index = 0) const override;
    virtual uint32_t GetDepthAttachmentRendererID() const override;
    virtual uint32_t GetEntityAttachmentRendererID() const override;
    virtual void ClearAttachment(uint32_t attachmentIndex, int value) override;
    virtual int ReadPixel(uint32_t attachmentIndex, int x, int y) override;
    virtual const FramebufferSpecification& GetSpecification() const override { return spec_; }

private:
    struct AttachmentInfo {
        ID3D11Texture2D* texture = nullptr;
        ID3D11RenderTargetView* rtv = nullptr;
        ID3D11ShaderResourceView* srv = nullptr;
        uint32_t rendererID = 0;
        FramebufferTextureSpecification spec;
    };

    void Invalidate();
    void Destroy();
    DXGI_FORMAT GetDXGIFormat(FramebufferTextureFormat format, bool forSRV = false) const;
    uint32_t GetAttachmentCount() const;

    FramebufferSpecification spec_;
    std::vector<AttachmentInfo> colorAttachments_;
    ID3D11Texture2D* depthTexture_ = nullptr;
    ID3D11DepthStencilView* depthDSV_ = nullptr;
    ID3D11ShaderResourceView* depthSRV_ = nullptr;
    uint32_t depthRendererID_ = 0;

    // MSAA resolved targets
    std::vector<ID3D11Texture2D*> resolvedTextures_;
    std::vector<ID3D11RenderTargetView*> resolvedRTVs_;
    std::vector<ID3D11ShaderResourceView*> resolvedSRVs_;
    std::vector<uint32_t> resolvedRendererIDs_;
    bool isMultisampled_ = false;
};

} // namespace renderer
} // namespace ge
