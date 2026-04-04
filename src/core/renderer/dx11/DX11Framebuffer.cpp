#include "DX11Framebuffer.h"
#include "DX11Context.h"
#include "../../debug/log.h"
#include "../../debug/assert.h"

#include <algorithm>
#include <cstring>

namespace ge {
namespace renderer {

DX11Framebuffer::DX11Framebuffer(const FramebufferSpecification& spec)
    : spec_(spec)
{
    GE_ASSERT(spec_.Attachments.Attachments.size() <= 4, "DX11Framebuffer: Max 4 color attachments supported");
    Invalidate();
}

DX11Framebuffer::~DX11Framebuffer()
{
    Destroy();
}

void DX11Framebuffer::Invalidate()
{
    Destroy();

    ID3D11Device* device = DX11Context::Get().GetDevice();
    isMultisampled_ = (spec_.Samples > 1);

    for (const auto& attachmentSpec : spec_.Attachments.Attachments) {
        if (attachmentSpec.TextureFormat == FramebufferTextureFormat::DEPTH24STENCIL8) {
            DXGI_FORMAT dxgiFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

            D3D11_TEXTURE2D_DESC texDesc = {};
            texDesc.Width = spec_.Width;
            texDesc.Height = spec_.Height;
            texDesc.MipLevels = 1;
            texDesc.ArraySize = 1;
            texDesc.Format = dxgiFormat;
            texDesc.SampleDesc.Count = spec_.Samples;
            texDesc.SampleDesc.Quality = 0;
            texDesc.Usage = D3D11_USAGE_DEFAULT;
            texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
            texDesc.CPUAccessFlags = 0;
            texDesc.MiscFlags = 0;

            HRESULT hr = device->CreateTexture2D(&texDesc, nullptr, &depthTexture_);
            if (FAILED(hr)) {
                GE_LOG_ERROR("DX11Framebuffer: Failed to create depth texture (HRESULT: 0x{0:X})", hr);
                continue;
            }

            D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
            dsvDesc.Format = dxgiFormat;
            dsvDesc.ViewDimension = isMultisampled_ ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;
            dsvDesc.Texture2D.MipSlice = 0;

            hr = device->CreateDepthStencilView(depthTexture_, &dsvDesc, &depthDSV_);
            if (FAILED(hr)) {
                GE_LOG_ERROR("DX11Framebuffer: Failed to create depth stencil view");
                continue;
            }

            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
            srvDesc.ViewDimension = isMultisampled_ ? D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Texture2D.MipLevels = 1;

            hr = device->CreateShaderResourceView(depthTexture_, &srvDesc, &depthSRV_);
            if (FAILED(hr)) {
                GE_LOG_ERROR("DX11Framebuffer: Failed to create depth SRV");
                continue;
            }

            depthRendererID_ = DX11Context::Get().RegisterSRV(depthSRV_);
        }
        else {
            DXGI_FORMAT texFormat = GetDXGIFormat(attachmentSpec.TextureFormat, false);
            DXGI_FORMAT srvFormat = GetDXGIFormat(attachmentSpec.TextureFormat, true);

            AttachmentInfo info;
            info.spec = attachmentSpec;

            D3D11_TEXTURE2D_DESC texDesc = {};
            texDesc.Width = spec_.Width;
            texDesc.Height = spec_.Height;
            texDesc.MipLevels = 1;
            texDesc.ArraySize = 1;
            texDesc.Format = texFormat;
            texDesc.SampleDesc.Count = spec_.Samples;
            texDesc.SampleDesc.Quality = 0;
            texDesc.Usage = D3D11_USAGE_DEFAULT;
            texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
            texDesc.CPUAccessFlags = 0;
            texDesc.MiscFlags = 0;

            HRESULT hr = device->CreateTexture2D(&texDesc, nullptr, &info.texture);
            if (FAILED(hr)) {
                GE_LOG_ERROR("DX11Framebuffer: Failed to create color texture (HRESULT: 0x{0:X})", hr);
                continue;
            }

            D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
            rtvDesc.Format = texFormat;
            rtvDesc.ViewDimension = isMultisampled_ ? D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D;
            rtvDesc.Texture2D.MipSlice = 0;

            hr = device->CreateRenderTargetView(info.texture, &rtvDesc, &info.rtv);
            if (FAILED(hr)) {
                GE_LOG_ERROR("DX11Framebuffer: Failed to create RTV");
                continue;
            }

            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = srvFormat;
            srvDesc.ViewDimension = isMultisampled_ ? D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Texture2D.MipLevels = 1;

            hr = device->CreateShaderResourceView(info.texture, &srvDesc, &info.srv);
            if (FAILED(hr)) {
                GE_LOG_ERROR("DX11Framebuffer: Failed to create SRV");
                continue;
            }

            info.rendererID = DX11Context::Get().RegisterSRV(info.srv);

            if (isMultisampled_) {
                D3D11_TEXTURE2D_DESC resolveDesc = {};
                resolveDesc.Width = spec_.Width;
                resolveDesc.Height = spec_.Height;
                resolveDesc.MipLevels = 1;
                resolveDesc.ArraySize = 1;
                resolveDesc.Format = texFormat;
                resolveDesc.SampleDesc.Count = 1;
                resolveDesc.SampleDesc.Quality = 0;
                resolveDesc.Usage = D3D11_USAGE_DEFAULT;
                resolveDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
                resolveDesc.CPUAccessFlags = 0;
                resolveDesc.MiscFlags = 0;

                ID3D11Texture2D* resolveTex = nullptr;
                hr = device->CreateTexture2D(&resolveDesc, nullptr, &resolveTex);
                if (FAILED(hr)) {
                    GE_LOG_ERROR("DX11Framebuffer: Failed to create resolve texture");
                    continue;
                }

                ID3D11RenderTargetView* resolveRTV = nullptr;
                D3D11_RENDER_TARGET_VIEW_DESC resolveRTVDesc = {};
                resolveRTVDesc.Format = texFormat;
                resolveRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                resolveRTVDesc.Texture2D.MipSlice = 0;
                hr = device->CreateRenderTargetView(resolveTex, &resolveRTVDesc, &resolveRTV);
                if (FAILED(hr)) {
                    GE_LOG_ERROR("DX11Framebuffer: Failed to create resolve RTV");
                    resolveTex->Release();
                    continue;
                }

                D3D11_SHADER_RESOURCE_VIEW_DESC resolveSRVDesc = {};
                resolveSRVDesc.Format = srvFormat;
                resolveSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                resolveSRVDesc.Texture2D.MostDetailedMip = 0;
                resolveSRVDesc.Texture2D.MipLevels = 1;
                ID3D11ShaderResourceView* resolveSRV = nullptr;
                hr = device->CreateShaderResourceView(resolveTex, &resolveSRVDesc, &resolveSRV);
                if (FAILED(hr)) {
                    GE_LOG_ERROR("DX11Framebuffer: Failed to create resolve SRV");
                    resolveTex->Release();
                    resolveRTV->Release();
                    continue;
                }

                uint32_t resolveID = DX11Context::Get().RegisterSRV(resolveSRV);

                resolvedTextures_.push_back(resolveTex);
                resolvedRTVs_.push_back(resolveRTV);
                resolvedSRVs_.push_back(resolveSRV);
                resolvedRendererIDs_.push_back(resolveID);
            }

            colorAttachments_.push_back(info);
        }
    }

    GE_LOG_INFO("DX11Framebuffer: Created {0}x{1} with {2} color attachments, {3} samples",
        spec_.Width, spec_.Height, colorAttachments_.size(), spec_.Samples);
}

void DX11Framebuffer::Destroy()
{
    auto& ctx = DX11Context::Get();

    for (auto& attachment : colorAttachments_) {
        if (attachment.srv) ctx.UnregisterSRV(attachment.rendererID);
        if (attachment.srv) attachment.srv->Release();
        if (attachment.rtv) attachment.rtv->Release();
        if (attachment.texture) attachment.texture->Release();
    }
    colorAttachments_.clear();

    if (depthSRV_) ctx.UnregisterSRV(depthRendererID_);
    if (depthSRV_) depthSRV_->Release();
    if (depthDSV_) depthDSV_->Release();
    if (depthTexture_) depthTexture_->Release();
    depthSRV_ = nullptr;
    depthDSV_ = nullptr;
    depthTexture_ = nullptr;
    depthRendererID_ = 0;

    for (size_t i = 0; i < resolvedTextures_.size(); ++i) {
        if (resolvedSRVs_[i]) ctx.UnregisterSRV(resolvedRendererIDs_[i]);
        if (resolvedSRVs_[i]) resolvedSRVs_[i]->Release();
        if (resolvedRTVs_[i]) resolvedRTVs_[i]->Release();
        if (resolvedTextures_[i]) resolvedTextures_[i]->Release();
    }
    resolvedTextures_.clear();
    resolvedRTVs_.clear();
    resolvedSRVs_.clear();
    resolvedRendererIDs_.clear();
}

void DX11Framebuffer::Bind()
{
    ID3D11DeviceContext* ctx = DX11Context::Get().GetDeviceContext();

    if (!colorAttachments_.empty()) {
        ID3D11RenderTargetView* rtvs[4] = {};
        for (size_t i = 0; i < colorAttachments_.size(); ++i) {
            rtvs[i] = colorAttachments_[i].rtv;
        }
        ctx->OMSetRenderTargets(static_cast<UINT>(colorAttachments_.size()), rtvs, depthDSV_);
    }
    else if (depthDSV_) {
        ctx->OMSetRenderTargets(0, nullptr, depthDSV_);
    }

    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = static_cast<float>(spec_.Width);
    viewport.Height = static_cast<float>(spec_.Height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    ctx->RSSetViewports(1, &viewport);
}

void DX11Framebuffer::Unbind()
{
    ID3D11DeviceContext* ctx = DX11Context::Get().GetDeviceContext();

    if (isMultisampled_) {
        for (size_t i = 0; i < colorAttachments_.size() && i < resolvedTextures_.size(); ++i) {
            DXGI_FORMAT format = GetDXGIFormat(colorAttachments_[i].spec.TextureFormat, false);
            ctx->ResolveSubresource(resolvedTextures_[i], 0, colorAttachments_[i].texture, 0, format);
        }
    }

    ID3D11RenderTargetView* nullRTVs[4] = {};
    ctx->OMSetRenderTargets(0, nullRTVs, nullptr);
}

void DX11Framebuffer::Resize(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0) {
        GE_LOG_WARN("DX11Framebuffer: Resize to 0x0, clamping to 1x1");
        width = std::max(width, 1u);
        height = std::max(height, 1u);
    }
    spec_.Width = width;
    spec_.Height = height;
    Invalidate();
}

uint32_t DX11Framebuffer::GetColorAttachmentRendererID(uint32_t index) const
{
    if (index >= colorAttachments_.size()) {
        GE_LOG_WARN("DX11Framebuffer: Color attachment index {0} out of range (max {1})", index, colorAttachments_.size());
        return 0;
    }

    if (isMultisampled_ && index < resolvedRendererIDs_.size()) {
        return resolvedRendererIDs_[index];
    }

    return colorAttachments_[index].rendererID;
}

uint32_t DX11Framebuffer::GetDepthAttachmentRendererID() const
{
    return depthRendererID_;
}

uint32_t DX11Framebuffer::GetEntityAttachmentRendererID() const
{
    for (size_t i = 0; i < colorAttachments_.size(); ++i) {
        if (colorAttachments_[i].spec.TextureFormat == FramebufferTextureFormat::RED_INTEGER) {
            return GetColorAttachmentRendererID(static_cast<uint32_t>(i));
        }
    }
    return 0;
}

void DX11Framebuffer::ClearAttachment(uint32_t attachmentIndex, int value)
{
    if (attachmentIndex >= colorAttachments_.size()) return;

    ID3D11DeviceContext* ctx = DX11Context::Get().GetDeviceContext();

    float clearColor[4] = {
        static_cast<float>(value) / 255.0f,
        static_cast<float>(value) / 255.0f,
        static_cast<float>(value) / 255.0f,
        1.0f
    };

    if (colorAttachments_[attachmentIndex].spec.TextureFormat == FramebufferTextureFormat::RED_INTEGER) {
        float intClear[4] = { static_cast<float>(value), 0.0f, 0.0f, 1.0f };
        ctx->ClearRenderTargetView(colorAttachments_[attachmentIndex].rtv, intClear);
    }
    else {
        ctx->ClearRenderTargetView(colorAttachments_[attachmentIndex].rtv, clearColor);
    }
}

int DX11Framebuffer::ReadPixel(uint32_t attachmentIndex, int x, int y)
{
    if (attachmentIndex >= colorAttachments_.size()) return -1;
    if (x < 0 || y < 0 || x >= static_cast<int>(spec_.Width) || y >= static_cast<int>(spec_.Height)) return -1;

    ID3D11Device* device = DX11Context::Get().GetDevice();
    ID3D11DeviceContext* ctx = DX11Context::Get().GetDeviceContext();

    ID3D11Texture2D* srcTexture = colorAttachments_[attachmentIndex].texture;
    if (isMultisampled_ && attachmentIndex < resolvedTextures_.size()) {
        srcTexture = resolvedTextures_[attachmentIndex];
    }

    D3D11_TEXTURE2D_DESC srcDesc;
    srcTexture->GetDesc(&srcDesc);

    D3D11_TEXTURE2D_DESC stagingDesc = srcDesc;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    stagingDesc.MiscFlags = 0;
    stagingDesc.SampleDesc.Count = 1;
    stagingDesc.SampleDesc.Quality = 0;

    ID3D11Texture2D* stagingTexture = nullptr;
    HRESULT hr = device->CreateTexture2D(&stagingDesc, nullptr, &stagingTexture);
    if (FAILED(hr)) {
        GE_LOG_ERROR("DX11Framebuffer: Failed to create staging texture for ReadPixel");
        return -1;
    }

    if (isMultisampled_ && attachmentIndex < resolvedTextures_.size()) {
        ctx->CopyResource(stagingTexture, resolvedTextures_[attachmentIndex]);
    }
    else {
        ctx->CopyResource(stagingTexture, srcTexture);
    }

    D3D11_MAPPED_SUBRESOURCE mapped;
    hr = ctx->Map(stagingTexture, 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(hr)) {
        GE_LOG_ERROR("DX11Framebuffer: Failed to map staging texture for ReadPixel");
        stagingTexture->Release();
        return -1;
    }

    int result = -1;
    if (colorAttachments_[attachmentIndex].spec.TextureFormat == FramebufferTextureFormat::RED_INTEGER) {
        uint32_t* pixelData = reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(mapped.pData) + y * mapped.RowPitch);
        result = static_cast<int>(pixelData[x]);
    }
    else {
        float* pixelData = reinterpret_cast<float*>(static_cast<uint8_t*>(mapped.pData) + y * mapped.RowPitch);
        result = static_cast<int>(pixelData[x * 4] * 255.0f);
    }

    ctx->Unmap(stagingTexture, 0);
    stagingTexture->Release();

    return result;
}

DXGI_FORMAT DX11Framebuffer::GetDXGIFormat(FramebufferTextureFormat format, bool forSRV) const
{
    (void)forSRV;
    switch (format) {
        case FramebufferTextureFormat::RGBA8:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case FramebufferTextureFormat::RGBA16F:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case FramebufferTextureFormat::RG16F:
            return DXGI_FORMAT_R16G16_FLOAT;
        case FramebufferTextureFormat::RED8:
            return DXGI_FORMAT_R8_UNORM;
        case FramebufferTextureFormat::RED_INTEGER:
            return DXGI_FORMAT_R8_UINT;
        case FramebufferTextureFormat::DEPTH24STENCIL8:
            return DXGI_FORMAT_D24_UNORM_S8_UINT;
        default:
            GE_LOG_WARN("DX11Framebuffer: Unknown texture format, defaulting to RGBA8");
            return DXGI_FORMAT_R8G8B8A8_UNORM;
    }
}

uint32_t DX11Framebuffer::GetAttachmentCount() const
{
    return static_cast<uint32_t>(colorAttachments_.size());
}

} // namespace renderer
} // namespace ge
