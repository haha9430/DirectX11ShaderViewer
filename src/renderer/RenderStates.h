#pragma once

#include <d3d11.h>
#include <wrl/client.h>

namespace dxsv
{
    class RenderStates
    {
    public:
        ID3D11SamplerState* linearSampler() const { return m_linearSampler.Get(); }

    private:
        Microsoft::WRL::ComPtr<ID3D11SamplerState> m_linearSampler;
    };
}
