#pragma once

#include "core/MathTypes.h"

namespace dxsv
{
    class Transform
    {
    public:
        Vector3 position{ 0.0f, 0.0f, 0.0f };
        Vector3 rotation{ 0.0f, 0.0f, 0.0f };
        Vector3 scale{ 1.0f, 1.0f, 1.0f };

        Matrix4 worldMatrix() const;
    };
}
