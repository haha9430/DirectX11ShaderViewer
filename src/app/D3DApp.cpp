#include "app/D3DApp.h"

#include "core/Logger.h"

#include <algorithm>
#include <combaseapi.h>
#include <exception>
#include <sstream>

namespace dxsv
{
    namespace
    {
        float clampValue(float value, float minValue, float maxValue)
        {
            return std::max(minValue, std::min(maxValue, value));
        }
    }

    D3DApp::~D3DApp()
    {
        if (m_comInitialized)
        {
            CoUninitialize();
        }
    }

    bool D3DApp::initialize(HINSTANCE instance, int showCommand)
    {
        try
        {
            const HRESULT comResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            if (SUCCEEDED(comResult))
            {
                m_comInitialized = true;
            }
            else if (comResult != RPC_E_CHANGED_MODE)
            {
                throwIfFailed(comResult, "CoInitializeEx failed.");
            }

            m_window.setInput(&m_input);
            if (!m_window.initialize(instance, showCommand, L"DirectX 11 Shader Viewer", 1280, 720))
            {
                return false;
            }

            m_scene.camera().setPerspective(DirectX::XMConvertToRadians(60.0f), static_cast<float>(m_window.width()) / static_cast<float>(m_window.height()), 0.1f, 100.0f);
            m_debugSettings.lightDirection = m_scene.light().direction;
            m_debugSettings.lightColor = m_scene.light().color;
            m_debugSettings.lightIntensity = m_scene.light().intensity * 4.0f;
            m_deviceResources.initialize(m_window.hwnd(), m_window.width(), m_window.height());
            m_renderer.initialize(&m_deviceResources);
            m_imguiLayer.initialize(m_window.hwnd(), m_deviceResources.device(), m_deviceResources.context());
            m_timer.reset();
            return true;
        }
        catch (const std::exception& ex)
        {
            Logger::error(widen(ex.what()));
            return false;
        }
    }

    int D3DApp::run()
    {
        while (m_window.processMessages())
        {
            update();
            render();
        }

        return 0;
    }

    void D3DApp::update()
    {
        m_timer.tick();
        handleResize();
        m_cameraController.update(m_scene.camera(), m_input, m_timer.deltaSeconds());
        updateDebugControls();
        refreshDebugTitle();
        m_input.endFrame();
    }

    void D3DApp::handleResize()
    {
        if (!m_window.consumeResize())
        {
            return;
        }

        m_deviceResources.resize(m_window.width(), m_window.height());
        m_renderer.resize();
        m_scene.camera().setPerspective(
            DirectX::XMConvertToRadians(60.0f),
            static_cast<float>(m_window.width()) / static_cast<float>(m_window.height()),
            0.1f,
            100.0f);
    }

    void D3DApp::updateDebugControls()
    {
        constexpr float kSmallStep = 0.01f;
        constexpr float kExposureStep = 0.02f;

        if (m_input.wasKeyPressed(VK_F1))
        {
            m_debugSettings.normalMapping = !m_debugSettings.normalMapping;
        }
        if (m_input.wasKeyPressed(VK_F2))
        {
            m_debugSettings.rotateObject = !m_debugSettings.rotateObject;
        }

        if (m_input.isKeyDown('Z'))
        {
            m_debugSettings.metallicFactor = clampValue(m_debugSettings.metallicFactor - kSmallStep, 0.0f, 2.0f);
        }
        if (m_input.isKeyDown('X'))
        {
            m_debugSettings.metallicFactor = clampValue(m_debugSettings.metallicFactor + kSmallStep, 0.0f, 2.0f);
        }
        if (m_input.isKeyDown('C'))
        {
            m_debugSettings.roughnessFactor = clampValue(m_debugSettings.roughnessFactor - kSmallStep, 0.05f, 2.0f);
        }
        if (m_input.isKeyDown('V'))
        {
            m_debugSettings.roughnessFactor = clampValue(m_debugSettings.roughnessFactor + kSmallStep, 0.05f, 2.0f);
        }
        if (m_input.isKeyDown('B'))
        {
            m_debugSettings.normalMapStrength = clampValue(m_debugSettings.normalMapStrength - kSmallStep, 0.0f, 2.0f);
        }
        if (m_input.isKeyDown('N'))
        {
            m_debugSettings.normalMapStrength = clampValue(m_debugSettings.normalMapStrength + kSmallStep, 0.0f, 2.0f);
        }
        if (m_input.isKeyDown('G'))
        {
            m_debugSettings.aoStrength = clampValue(m_debugSettings.aoStrength - kSmallStep, 0.0f, 2.0f);
        }
        if (m_input.isKeyDown('H'))
        {
            m_debugSettings.aoStrength = clampValue(m_debugSettings.aoStrength + kSmallStep, 0.0f, 2.0f);
        }
        if (m_input.isKeyDown(VK_OEM_MINUS))
        {
            m_debugSettings.exposure = clampValue(m_debugSettings.exposure - kExposureStep, 0.1f, 5.0f);
        }
        if (m_input.isKeyDown(VK_OEM_PLUS))
        {
            m_debugSettings.exposure = clampValue(m_debugSettings.exposure + kExposureStep, 0.1f, 5.0f);
        }
    }

    void D3DApp::refreshDebugTitle()
    {
        m_debugTitleTimer += m_timer.deltaSeconds();
        if (m_debugTitleTimer < 0.1f)
        {
            return;
        }

        m_debugTitleTimer = 0.0f;
        std::wstringstream title;
        title.precision(2);
        title << std::fixed
            << L"DirectX 11 Shader Viewer | "
            << L"Metal Z/X " << m_debugSettings.metallicFactor << L" | "
            << L"Rough C/V " << m_debugSettings.roughnessFactor << L" | "
            << L"Normal B/N " << m_debugSettings.normalMapStrength << L" "
            << (m_debugSettings.normalMapping ? L"on" : L"off") << L" | "
            << L"AO G/H " << m_debugSettings.aoStrength << L" | "
            << L"Exposure -/+ " << m_debugSettings.exposure << L" | "
            << L"Light " << m_debugSettings.lightIntensity << L" | "
            << L"F1 normal F2 rotate";
        m_window.setTitle(title.str());
    }

    void D3DApp::render()
    {
        m_renderer.render(m_timer.totalSeconds(), m_scene.camera(), m_debugSettings);
        m_imguiLayer.beginFrame();
        m_imguiLayer.draw(m_debugSettings, m_renderer);
        m_imguiLayer.endFrame();
        m_deviceResources.present();
    }
}
