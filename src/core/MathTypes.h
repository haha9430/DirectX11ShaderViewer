#pragma once

#include <DirectXMath.h>

namespace dxsv
{
    using Vector2 = DirectX::XMFLOAT2;
    using Vector3 = DirectX::XMFLOAT3;
    using Vector4 = DirectX::XMFLOAT4;
    using Matrix4 = DirectX::XMFLOAT4X4;

    inline Matrix4 identityMatrix()
    {
        Matrix4 result{};
        DirectX::XMStoreFloat4x4(&result, DirectX::XMMatrixIdentity());
        return result;
    }
}
