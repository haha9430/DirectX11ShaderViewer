#pragma once

#include "app/WinApp.h"
#include "debug/DebugSettings.h"
#include "debug/ImGuiLayer.h"
#include "core/Input.h"
#include "core/Timer.h"
#include "renderer/DeviceResources.h"
#include "renderer/Renderer.h"
#include "scene/Scene.h"
#include "systems/CameraController.h"

namespace dxsv
{
    class D3DApp
    {
    public:
        ~D3DApp();

        bool initialize(HINSTANCE instance, int showCommand);
        int run();

    private:
        void update();
        void handleResize();
        void updateDebugControls();
        void refreshDebugTitle();
        void render();

        WinApp m_window;
        Input m_input;
        Timer m_timer;
        Scene m_scene;
        DebugSettings m_debugSettings;
        ImGuiLayer m_imguiLayer;
        CameraController m_cameraController;
        DeviceResources m_deviceResources;
        Renderer m_renderer;
        bool m_comInitialized = false;
        float m_debugTitleTimer = 0.0f;
    };
}
