#include "app/WinApp.h"

#include "core/Logger.h"

#include <backends/imgui_impl_win32.h>
#include <windowsx.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

namespace dxsv
{
    bool WinApp::initialize(HINSTANCE instance, int showCommand, const std::wstring& title, int width, int height)
    {
        m_instance = instance;
        m_width = width;
        m_height = height;

        constexpr wchar_t kWindowClassName[] = L"DirectX11ShaderViewerWindow";

        WNDCLASSEXW windowClass{};
        windowClass.cbSize = sizeof(WNDCLASSEXW);
        windowClass.style = CS_HREDRAW | CS_VREDRAW;
        windowClass.lpfnWndProc = WinApp::windowProc;
        windowClass.hInstance = m_instance;
        windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
        windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        windowClass.lpszClassName = kWindowClassName;

        if (!RegisterClassExW(&windowClass))
        {
            Logger::error(L"RegisterClassExW failed.");
            return false;
        }

        RECT windowRect{ 0, 0, m_width, m_height };
        AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

        m_hwnd = CreateWindowExW(
            0,
            kWindowClassName,
            title.c_str(),
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            windowRect.right - windowRect.left,
            windowRect.bottom - windowRect.top,
            nullptr,
            nullptr,
            m_instance,
            this);

        if (!m_hwnd)
        {
            Logger::error(L"CreateWindowExW failed.");
            return false;
        }

        ShowWindow(m_hwnd, showCommand);
        UpdateWindow(m_hwnd);
        return true;
    }

    void WinApp::setTitle(const std::wstring& title)
    {
        if (m_hwnd)
        {
            SetWindowTextW(m_hwnd, title.c_str());
        }
    }

    bool WinApp::processMessages()
    {
        MSG message{};
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
        {
            if (message.message == WM_QUIT)
            {
                return false;
            }

            TranslateMessage(&message);
            DispatchMessageW(&message);
        }

        return true;
    }

    bool WinApp::consumeResize()
    {
        const bool wasResized = m_resized;
        m_resized = false;
        return wasResized;
    }

    LRESULT CALLBACK WinApp::windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (ImGui_ImplWin32_WndProcHandler(hwnd, message, wParam, lParam))
        {
            return TRUE;
        }

        WinApp* app = reinterpret_cast<WinApp*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

        switch (message)
        {
        case WM_NCCREATE:
        {
            const auto createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));
            return TRUE;
        }
        case WM_SIZE:
            if (app && wParam != SIZE_MINIMIZED)
            {
                const int width = LOWORD(lParam);
                const int height = HIWORD(lParam);
                if (width > 0 && height > 0 && (width != app->m_width || height != app->m_height))
                {
                    app->m_width = width;
                    app->m_height = height;
                    app->m_resized = true;
                }
            }
            return 0;
        case WM_KEYDOWN:
            if (app && app->m_input && wParam < 256)
            {
                app->m_input->onKeyDown(static_cast<unsigned int>(wParam));
            }
            return 0;
        case WM_KEYUP:
            if (app && app->m_input && wParam < 256)
            {
                app->m_input->onKeyUp(static_cast<unsigned int>(wParam));
            }
            return 0;
        case WM_MOUSEMOVE:
            if (app && app->m_input)
            {
                app->m_input->onMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            }
            return 0;
        case WM_RBUTTONDOWN:
            if (app && app->m_input)
            {
                app->m_input->onKeyDown(VK_RBUTTON);
                SetCapture(hwnd);
            }
            return 0;
        case WM_RBUTTONUP:
            if (app && app->m_input)
            {
                app->m_input->onKeyUp(VK_RBUTTON);
                ReleaseCapture();
            }
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcW(hwnd, message, wParam, lParam);
        }
    }
}
