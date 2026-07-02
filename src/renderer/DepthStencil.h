#pragma once

#include <d3d11.h>
#include <wrl/client.h>

namespace dxsv
{
    class DepthStencil
    {
    public:
        ID3D11DepthStencilView* view() const { return m_view.Get(); }

    private:
        Microsoft::WRL::ComPtr<ID3D11Texture2D> m_buffer;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_view;
    };
}
