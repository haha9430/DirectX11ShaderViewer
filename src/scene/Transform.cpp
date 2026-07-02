#include "scene/Transform.h"

using namespace DirectX;

namespace dxsv
{
    Matrix4 Transform::worldMatrix() const
    {
        const XMMATRIX scaleMatrix = XMMatrixScaling(scale.x, scale.y, scale.z);
        const XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
        const XMMATRIX translationMatrix = XMMatrixTranslation(position.x, position.y, position.z);

        Matrix4 result{};
        XMStoreFloat4x4(&result, scaleMatrix * rotationMatrix * translationMatrix);
        return result;
    }
}
