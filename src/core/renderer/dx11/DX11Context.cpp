#include "DX11Context.h"
#include "../../debug/log.h"
#include "../../debug/assert.h"

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace ge {
namespace renderer {

static DX11Context* s_DX11ContextInstance = nullptr;

DX11Context& DX11Context::Get()
{
    GE_ASSERT(s_DX11ContextInstance, "DX11Context not initialized!");
    return *s_DX11ContextInstance;
}

DX11Context::DX11Context(void* windowHandle)
    : windowHandle_(windowHandle)
{
    s_DX11ContextInstance = this;
}

DX11Context::~DX11Context()
{
    if (renderTargetView_) renderTargetView_->Release();
    if (depthStencilView_) depthStencilView_->Release();
    if (depthStencilBuffer_) depthStencilBuffer_->Release();
    if (swapChain_) swapChain_->Release();
    if (deviceContext_) deviceContext_->Release();
    if (device_) device_->Release();
}

void DX11Context::Init()
{
    GE_LOG_INFO("Initializing DX11 Context");

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = static_cast<HWND>(windowHandle_);
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sd.Flags = 0;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    D3D_FEATURE_LEVEL featureLevel;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        createDeviceFlags,
        featureLevels,
        ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION,
        &sd,
        &swapChain_,
        &device_,
        &featureLevel,
        &deviceContext_
    );

    if (FAILED(hr)) {
        GE_LOG_ERROR("Failed to create DX11 device and swap chain (HRESULT: 0x{0:X})", hr);
        return;
    }

    GE_LOG_INFO("DX11 Feature Level: {0}", 
        featureLevel == D3D_FEATURE_LEVEL_11_1 ? "11.1" :
        featureLevel == D3D_FEATURE_LEVEL_11_0 ? "11.0" :
        featureLevel == D3D_FEATURE_LEVEL_10_1 ? "10.1" : "10.0");

    // Create render target view
    ID3D11Texture2D* backBuffer = nullptr;
    hr = swapChain_->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (FAILED(hr)) {
        GE_LOG_ERROR("Failed to get swap chain back buffer");
        return;
    }

    hr = device_->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView_);
    backBuffer->Release();
    if (FAILED(hr)) {
        GE_LOG_ERROR("Failed to create render target view");
        return;
    }

    // Create depth stencil
    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width = 1920;
    depthDesc.Height = 1080;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.SampleDesc.Quality = 0;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    hr = device_->CreateTexture2D(&depthDesc, nullptr, &depthStencilBuffer_);
    if (FAILED(hr)) {
        GE_LOG_ERROR("Failed to create depth stencil buffer");
        return;
    }

    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = depthDesc.Format;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;

    hr = device_->CreateDepthStencilView(depthStencilBuffer_, &dsvDesc, &depthStencilView_);
    if (FAILED(hr)) {
        GE_LOG_ERROR("Failed to create depth stencil view");
        return;
    }

    // Set render targets
    deviceContext_->OMSetRenderTargets(1, &renderTargetView_, depthStencilView_);

    // Set viewport
    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = 1920.0f;
    viewport.Height = 1080.0f;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    deviceContext_->RSSetViewports(1, &viewport);

    GE_LOG_INFO("DX11 Context initialized successfully");
}

void DX11Context::SwapBuffers()
{
    if (swapChain_) {
        swapChain_->Present(1, 0);
    }
}

} // namespace renderer
} // namespace ge
