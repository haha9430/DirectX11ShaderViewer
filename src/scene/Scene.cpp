#include "scene/Scene.h"

namespace dxsv
{
    Scene::Scene()
    {
        m_camera.lookAt({ 0.0f, 1.5f, -5.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });
        m_camera.setPerspective(DirectX::XMConvertToRadians(60.0f), 16.0f / 9.0f, 0.1f, 100.0f);
        m_objects.emplace_back("Preview Object");
    }
}
