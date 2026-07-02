#pragma once

#include "scene/Transform.h"

#include <string>

namespace dxsv
{
    class GameObject
    {
    public:
        explicit GameObject(std::string name = {});

        const std::string& name() const { return m_name; }
        Transform& transform() { return m_transform; }
        const Transform& transform() const { return m_transform; }

    private:
        std::string m_name;
        Transform m_transform;
    };
}
