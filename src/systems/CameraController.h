#pragma once

#include "core/Input.h"
#include "scene/Camera.h"

namespace dxsv
{
    class CameraController
    {
    public:
        void update(Camera& camera, const Input& input, float deltaSeconds);

    private:
        float m_moveSpeed = 4.0f;
        float m_mouseSensitivity = 0.003f;
    };
}
