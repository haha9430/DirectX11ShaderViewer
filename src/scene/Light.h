#pragma once

#include "core/MathTypes.h"

namespace dxsv
{
    struct DirectionalLight
    {
        Vector3 direction{ -0.35f, 1.0f, -0.25f };
        float intensity = 1.0f;
        Vector3 color{ 1.0f, 1.0f, 1.0f };
        float specularPower = 32.0f;
    };
}
