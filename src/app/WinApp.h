#pragma once

#include "core/Common.h"
#include "core/Input.h"

#include <string>

namespace dxsv
{
    class WinApp
    {
    public:
        bool initialize(HINSTANCE instance, int showCommand, const std::wstring& title, int width, int height);
        void setInput(Input* input) { m_input = input; }
        void setTitle(const std::wstring& title);
        bool processMessages();
        bool consumeResize();

        HWND hwnd() const { return m_hwnd; }
        int width() const { return m_width; }
        int height() const { return m_height; }

    private:
        static LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

        HINSTANCE m_instance = nullptr;
        HWND m_hwnd = nullptr;
        Input* m_input = nullptr;
        int m_width = 1280;
        int m_height = 720;
        bool m_resized = false;
    };
}
