#pragma once

#include "core/Common.h"

#include <d3d11.h>
#include <wrl/client.h>

namespace dxsv
{
    class ConstantBuffer
    {
    public:
        void reset() { m_buffer.Reset(); }
        ID3D11Buffer* get() const { return m_buffer.Get(); }

    private:
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_buffer;
    };
}
