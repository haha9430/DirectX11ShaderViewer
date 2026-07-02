#include "core/Input.h"

namespace dxsv
{
    void Input::onKeyDown(unsigned int key)
    {
        if (key < m_keys.size())
        {
            if (!m_keys[key])
            {
                m_pressedKeys[key] = true;
            }
            m_keys[key] = true;
        }
    }

    void Input::onKeyUp(unsigned int key)
    {
        if (key < m_keys.size())
        {
            m_keys[key] = false;
        }
    }

    void Input::onMouseMove(int x, int y)
    {
        if (m_hasMousePosition)
        {
            m_mouseDeltaX += x - m_mouseX;
            m_mouseDeltaY += y - m_mouseY;
        }

        m_mouseX = x;
        m_mouseY = y;
        m_hasMousePosition = true;
    }

    void Input::endFrame()
    {
        m_pressedKeys.fill(false);
        m_mouseDeltaX = 0;
        m_mouseDeltaY = 0;
    }

    bool Input::isKeyDown(unsigned int key) const
    {
        return key < m_keys.size() && m_keys[key];
    }

    bool Input::wasKeyPressed(unsigned int key) const
    {
        return key < m_pressedKeys.size() && m_pressedKeys[key];
    }
}
