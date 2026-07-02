#pragma once

#include "core/MathTypes.h"

namespace dxsv
{
    class Camera
    {
    public:
        void setPerspective(float fovYRadians, float aspectRatio, float nearPlane, float farPlane);
        void lookAt(const Vector3& eye, const Vector3& target, const Vector3& up);
        void setPosition(const Vector3& position);
        void setRotation(float yawRadians, float pitchRadians);
        void moveLocal(float right, float up, float forward);

        float yaw() const { return m_yaw; }
        float pitch() const { return m_pitch; }

        const Matrix4& view() const { return m_view; }
        const Matrix4& projection() const { return m_projection; }
        const Vector3& position() const { return m_position; }

    private:
        void updateView();

        Vector3 m_position{ 0.0f, 0.0f, -5.0f };
        float m_yaw = 0.0f;
        float m_pitch = 0.0f;
        Matrix4 m_view = identityMatrix();
        Matrix4 m_projection = identityMatrix();
    };
}
