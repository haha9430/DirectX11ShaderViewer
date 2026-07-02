#include "scene/GameObject.h"

#include <utility>

namespace dxsv
{
    GameObject::GameObject(std::string name)
        : m_name(std::move(name))
    {
    }
}
