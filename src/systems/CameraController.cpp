#include "systems/CameraController.h"

#include <Windows.h>

namespace dxsv
{
    void CameraController::update(Camera& camera, const Input& input, float deltaSeconds)
    {
        float right = 0.0f;
        float up = 0.0f;
        float forward = 0.0f;

        if (input.isKeyDown('W'))
        {
            forward += 1.0f;
        }
        if (input.isKeyDown('S'))
        {
            forward -= 1.0f;
        }
        if (input.isKeyDown('D'))
        {
            right += 1.0f;
        }
        if (input.isKeyDown('A'))
        {
            right -= 1.0f;
        }
        if (input.isKeyDown(VK_SPACE))
        {
            up += 1.0f;
        }
        if (input.isKeyDown(VK_CONTROL))
        {
            up -= 1.0f;
        }

        const float speed = (input.isKeyDown(VK_SHIFT) ? m_moveSpeed * 2.5f : m_moveSpeed) * deltaSeconds;
        camera.moveLocal(right * speed, up * speed, forward * speed);

        if (input.isKeyDown(VK_RBUTTON))
        {
            const float yaw = camera.yaw() + static_cast<float>(input.mouseDeltaX()) * m_mouseSensitivity;
            const float pitch = camera.pitch() + static_cast<float>(input.mouseDeltaY()) * m_mouseSensitivity;
            camera.setRotation(yaw, pitch);
        }
    }
}
