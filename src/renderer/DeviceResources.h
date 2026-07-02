#pragma once

#include "core/Common.h"

#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>

namespace dxsv
{
    class DeviceResources
    {
    public:
        void initialize(HWND hwnd, int width, int height);
        void resize(int width, int height);
        void clear(const float color[4]);
        void setBackBufferRenderTarget();
        void present();

        ID3D11Device* device() const { return m_device.Get(); }
        ID3D11DeviceContext* context() const { return m_context.Get(); }
        ID3D11RenderTargetView* renderTargetView() const { return m_renderTargetView.Get(); }
        ID3D11DepthStencilView* depthStencilView() const { return m_depthStencilView.Get(); }
        int width() const { return m_width; }
        int height() const { return m_height; }

    private:
        void createDeviceAndSwapChain(HWND hwnd, int width, int height);
        void createBackBufferResources(int width, int height);

        Microsoft::WRL::ComPtr<ID3D11Device> m_device;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context;
        Microsoft::WRL::ComPtr<IDXGISwapChain> m_swapChain;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_renderTargetView;
        Microsoft::WRL::ComPtr<ID3D11Texture2D> m_depthStencilBuffer;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_depthStencilView;
        D3D11_VIEWPORT m_viewport{};
        int m_width = 0;
        int m_height = 0;
    };
}
