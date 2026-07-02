#pragma once

#include <d3d11.h>
#include <wrl/client.h>

namespace dxsv
{
    class RenderTarget
    {
    public:
        ID3D11RenderTargetView* view() const { return m_view.Get(); }
        ID3D11ShaderResourceView* shaderResource() const { return m_shaderResource.Get(); }

    private:
        Microsoft::WRL::ComPtr<ID3D11Texture2D> m_texture;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_view;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_shaderResource;
    };
}