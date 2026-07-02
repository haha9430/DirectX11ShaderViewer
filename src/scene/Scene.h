#pragma once

#include "scene/Camera.h"
#include "scene/GameObject.h"
#include "scene/Light.h"

#include <vector>

namespace dxsv
{
    class Scene
    {
    public:
        Scene();

        Camera& camera() { return m_camera; }
        DirectionalLight& light() { return m_light; }
        std::vector<GameObject>& objects() { return m_objects; }

    private:
        Camera m_camera;
        DirectionalLight m_light;
        std::vector<GameObject> m_objects;
    };
}
