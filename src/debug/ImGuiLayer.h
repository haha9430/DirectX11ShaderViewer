#pragma once

#include "core/Common.h"
#include "debug/DebugSettings.h"

#include <d3d11.h>

namespace dxsv
{
    class Renderer;

    class ImGuiLayer
    {
    public:
        ~ImGuiLayer();

        void initialize(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context);
        void shutdown();
        void beginFrame();
        void draw(DebugSettings& settings, const Renderer& renderer);
        void endFrame();

        bool isInitialized() const { return m_initialized; }

    private:
        bool m_initialized = false;
    };
}
