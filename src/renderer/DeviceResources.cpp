#include "renderer/DeviceResources.h"

namespace dxsv
{
    void DeviceResources::initialize(HWND hwnd, int width, int height)
    {
        m_width = width;
        m_height = height;
        createDeviceAndSwapChain(hwnd, width, height);
        createBackBufferResources(width, height);
    }

    void DeviceResources::resize(int width, int height)
    {
        if (width <= 0 || height <= 0 || (width == m_width && height == m_height))
        {
            return;
        }

        m_width = width;
        m_height = height;

        ID3D11ShaderResourceView* nullSrvs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = {};
        m_context->OMSetRenderTargets(0, nullptr, nullptr);
        m_context->PSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, nullSrvs);
        m_renderTargetView.Reset();
        m_depthStencilView.Reset();
        m_depthStencilBuffer.Reset();
        m_context->Flush();

        throwIfFailed(
            m_swapChain->ResizeBuffers(0, static_cast<UINT>(width), static_cast<UINT>(height), DXGI_FORMAT_UNKNOWN, 0),
            "IDXGISwapChain::ResizeBuffers failed.");

        createBackBufferResources(width, height);
    }

    void DeviceResources::clear(const float color[4])
    {
        m_context->ClearRenderTargetView(m_renderTargetView.Get(), color);
        m_context->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
        ID3D11RenderTargetView* targets[] = { m_renderTargetView.Get() };
        m_context->OMSetRenderTargets(1, targets, m_depthStencilView.Get());
    }

    void DeviceResources::setBackBufferRenderTarget()
    {
        ID3D11RenderTargetView* targets[] = { m_renderTargetView.Get() };
        m_context->OMSetRenderTargets(1, targets, nullptr);
        m_context->RSSetViewports(1, &m_viewport);
    }

    void DeviceResources::present()
    {
        m_swapChain->Present(1, 0);
    }

    void DeviceResources::createDeviceAndSwapChain(HWND hwnd, int width, int height)
    {
        DXGI_SWAP_CHAIN_DESC swapChainDesc{};
        swapChainDesc.BufferCount = 1;
        swapChainDesc.BufferDesc.Width = static_cast<UINT>(width);
        swapChainDesc.BufferDesc.Height = static_cast<UINT>(height);
        swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
        swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.OutputWindow = hwnd;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.Windowed = TRUE;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        UINT creationFlags = 0;
#if defined(_DEBUG)
        creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
        D3D_FEATURE_LEVEL selectedFeatureLevel{};

        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            creationFlags,
            featureLevels,
            1,
            D3D11_SDK_VERSION,
            &swapChainDesc,
            m_swapChain.GetAddressOf(),
            m_device.GetAddressOf(),
            &selectedFeatureLevel,
            m_context.GetAddressOf());

#if defined(_DEBUG)
        if (hr == DXGI_ERROR_SDK_COMPONENT_MISSING)
        {
            creationFlags &= ~D3D11_CREATE_DEVICE_DEBUG;
            hr = D3D11CreateDeviceAndSwapChain(
                nullptr,
                D3D_DRIVER_TYPE_HARDWARE,
                nullptr,
                creationFlags,
                featureLevels,
                1,
                D3D11_SDK_VERSION,
                &swapChainDesc,
                m_swapChain.GetAddressOf(),
                m_device.GetAddressOf(),
                &selectedFeatureLevel,
                m_context.GetAddressOf());
        }
#endif

        throwIfFailed(hr, "D3D11CreateDeviceAndSwapChain failed.");
    }

    void DeviceResources::createBackBufferResources(int width, int height)
    {
        Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
        throwIfFailed(
            m_swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf())),
            "IDXGISwapChain::GetBuffer failed.");

        throwIfFailed(
            m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, m_renderTargetView.GetAddressOf()),
            "CreateRenderTargetView failed.");

        D3D11_TEXTURE2D_DESC depthDesc{};
        depthDesc.Width = static_cast<UINT>(width);
        depthDesc.Height = static_cast<UINT>(height);
        depthDesc.MipLevels = 1;
        depthDesc.ArraySize = 1;
        depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthDesc.SampleDesc.Count = 1;
        depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

        throwIfFailed(
            m_device->CreateTexture2D(&depthDesc, nullptr, m_depthStencilBuffer.GetAddressOf()),
            "CreateTexture2D depth buffer failed.");

        throwIfFailed(
            m_device->CreateDepthStencilView(m_depthStencilBuffer.Get(), nullptr, m_depthStencilView.GetAddressOf()),
            "CreateDepthStencilView failed.");

        m_viewport.TopLeftX = 0.0f;
        m_viewport.TopLeftY = 0.0f;
        m_viewport.Width = static_cast<float>(width);
        m_viewport.Height = static_cast<float>(height);
        m_viewport.MinDepth = 0.0f;
        m_viewport.MaxDepth = 1.0f;
        m_context->RSSetViewports(1, &m_viewport);
    }
}
