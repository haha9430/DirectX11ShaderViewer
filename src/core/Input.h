#pragma once

#include <array>
#include <Windows.h>

namespace dxsv
{
    class Input
    {
    public:
        void onKeyDown(unsigned int key);
        void onKeyUp(unsigned int key);
        void onMouseMove(int x, int y);
        void endFrame();

        bool isKeyDown(unsigned int key) const;
        bool wasKeyPressed(unsigned int key) const;
        int mouseDeltaX() const { return m_mouseDeltaX; }
        int mouseDeltaY() const { return m_mouseDeltaY; }
        bool hasMousePosition() const { return m_hasMousePosition; }

    private:
        std::array<bool, 256> m_keys{};
        std::array<bool, 256> m_pressedKeys{};
        bool m_hasMousePosition = false;
        int m_mouseX = 0;
        int m_mouseY = 0;
        int m_mouseDeltaX = 0;
        int m_mouseDeltaY = 0;
    };
}
