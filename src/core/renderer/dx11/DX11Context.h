#pragma once

#include "../GraphicsContext.h"
#include <d3d11.h>
#include <dxgi.h>
#include <unordered_map>
#include <cstdint>

namespace ge {
namespace renderer {

    class DX11Context : public GraphicsContext
    {
    public:
        DX11Context(void* windowHandle);
        virtual ~DX11Context() override;

        virtual void Init() override;
        virtual void SwapBuffers() override;

        static DX11Context& Get();

        ID3D11Device* GetDevice() const { return device_; }
        ID3D11DeviceContext* GetDeviceContext() const { return deviceContext_; }
        IDXGISwapChain* GetSwapChain() const { return swapChain_; }

        // SRV Registry for texture handle abstraction
        uint32_t RegisterSRV(ID3D11ShaderResourceView* srv);
        void UnregisterSRV(uint32_t id);
        ID3D11ShaderResourceView* GetSRV(uint32_t id) const;

    private:
        void* windowHandle_;
        ID3D11Device* device_ = nullptr;
        ID3D11DeviceContext* deviceContext_ = nullptr;
        IDXGISwapChain* swapChain_ = nullptr;
        ID3D11RenderTargetView* renderTargetView_ = nullptr;
        ID3D11DepthStencilView* depthStencilView_ = nullptr;
        ID3D11Texture2D* depthStencilBuffer_ = nullptr;

        std::unordered_map<uint32_t, ID3D11ShaderResourceView*> srvRegistry_;
        uint32_t nextSRVId_ = 1;
    };

} // namespace renderer
} // namespace ge
