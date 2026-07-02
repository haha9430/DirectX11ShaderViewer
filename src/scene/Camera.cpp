#include "scene/Camera.h"

#include <algorithm>
#include <cmath>

using namespace DirectX;

namespace dxsv
{
    void Camera::setPerspective(float fovYRadians, float aspectRatio, float nearPlane, float farPlane)
    {
        XMStoreFloat4x4(&m_projection, XMMatrixPerspectiveFovLH(fovYRadians, aspectRatio, nearPlane, farPlane));
    }

    void Camera::lookAt(const Vector3& eye, const Vector3& target, const Vector3& up)
    {
        m_position = eye;
        const XMVECTOR eyeVector = XMLoadFloat3(&eye);
        const XMVECTOR targetVector = XMLoadFloat3(&target);
        const XMVECTOR upVector = XMLoadFloat3(&up);
        XMStoreFloat4x4(&m_view, XMMatrixLookAtLH(eyeVector, targetVector, upVector));

        const XMVECTOR direction = XMVector3Normalize(targetVector - eyeVector);
        XMFLOAT3 dir{};
        XMStoreFloat3(&dir, direction);
        m_yaw = std::atan2(dir.x, dir.z);
        m_pitch = std::asin(dir.y);
    }

    void Camera::setPosition(const Vector3& position)
    {
        m_position = position;
        updateView();
    }

    void Camera::setRotation(float yawRadians, float pitchRadians)
    {
        constexpr float kPitchLimit = XM_PIDIV2 - 0.01f;
        m_yaw = yawRadians;
        m_pitch = std::max(-kPitchLimit, std::min(kPitchLimit, pitchRadians));
        updateView();
    }

    void Camera::moveLocal(float right, float up, float forward)
    {
        const XMMATRIX rotation = XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, 0.0f);
        const XMVECTOR rightVector = XMVector3TransformNormal(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), rotation);
        const XMVECTOR upVector = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        const XMVECTOR forwardVector = XMVector3TransformNormal(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotation);

        XMVECTOR position = XMLoadFloat3(&m_position);
        position += rightVector * right;
        position += upVector * up;
        position += forwardVector * forward;
        XMStoreFloat3(&m_position, position);
        updateView();
    }

    void Camera::updateView()
    {
        const XMMATRIX rotation = XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, 0.0f);
        const XMVECTOR eye = XMLoadFloat3(&m_position);
        const XMVECTOR forward = XMVector3TransformNormal(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotation);
        const XMVECTOR up = XMVector3TransformNormal(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), rotation);
        XMStoreFloat4x4(&m_view, XMMatrixLookAtLH(eye, eye + forward, up));
    }
}
