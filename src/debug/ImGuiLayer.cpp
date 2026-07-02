#include "debug/ImGuiLayer.h"

#include "renderer/Renderer.h"

#include <imgui.h>
#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>

namespace dxsv
{
    ImGuiLayer::~ImGuiLayer()
    {
        shutdown();
    }

    void ImGuiLayer::initialize(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context)
    {
        if (m_initialized)
        {
            return;
        }

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        ImGui::StyleColorsDark();

        ImGui_ImplWin32_Init(hwnd);
        ImGui_ImplDX11_Init(device, context);
        m_initialized = true;
    }

    void ImGuiLayer::shutdown()
    {
        if (!m_initialized)
        {
            return;
        }

        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        m_initialized = false;
    }

    void ImGuiLayer::beginFrame()
    {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    void ImGuiLayer::draw(DebugSettings& settings, const Renderer& renderer)
    {
        ImGui::SetNextWindowPos(ImVec2(16.0f, 16.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(340.0f, 0.0f), ImGuiCond_FirstUseEver);

        ImGui::Begin("PBR Debug");
        ImGui::TextUnformatted("View");
        const char* debugViewItems[] = {
            "Final PBR",
            "Albedo",
            "Normal",
            "Metallic",
            "Roughness",
            "AO",
            "Direct Lighting",
            "Diffuse IBL",
            "Specular IBL",
            "Irradiance",
            "Prefiltered Specular",
            "BRDF LUT",
        };
        int debugView = static_cast<int>(settings.pbrDebugView);
        if (ImGui::Combo("Debug View", &debugView, debugViewItems, IM_ARRAYSIZE(debugViewItems)))
        {
            settings.pbrDebugView = static_cast<PbrDebugView>(debugView);
        }

        ImGui::Separator();
        ImGui::TextUnformatted("Material");
        const char* textureSets[] = { "Silver", "Light Gold", "Bronze" };
        int textureSet = static_cast<int>(settings.textureMaterialSet);
        if (ImGui::Combo("Texture Set", &textureSet, textureSets, IM_ARRAYSIZE(textureSets)))
        {
            settings.textureMaterialSet = static_cast<TextureMaterialSet>(textureSet);
        }

        ImGui::ColorEdit3("Tint", &settings.baseColor.x);
        ImGui::SliderFloat("Metallic", &settings.metallicFactor, 0.0f, 2.0f, "%.2f");
        ImGui::SliderFloat("Roughness", &settings.roughnessFactor, 0.05f, 2.0f, "%.2f");
        ImGui::SliderFloat("AO Strength", &settings.aoStrength, 0.0f, 2.0f, "%.2f");
        ImGui::Checkbox("Normal Mapping", &settings.normalMapping);
        ImGui::SliderFloat("Normal Strength", &settings.normalMapStrength, 0.0f, 2.0f, "%.2f");

        ImGui::Separator();
        ImGui::TextUnformatted("Lighting");
        ImGui::SliderFloat3("Light Direction", &settings.lightDirection.x, -1.0f, 1.0f, "%.2f");
        ImGui::ColorEdit3("Light Color", &settings.lightColor.x);
        ImGui::SliderFloat("Direct Light", &settings.lightIntensity, 0.0f, 20.0f, "%.2f");

        ImGui::Separator();
        ImGui::TextUnformatted("Environment");
        ImGui::Checkbox("IBL", &settings.iblEnabled);
        ImGui::SliderFloat("Diffuse IBL", &settings.diffuseIblStrength, 0.0f, 5.0f, "%.2f");
        ImGui::SliderFloat("Specular IBL", &settings.specularIblStrength, 0.0f, 5.0f, "%.2f");
        ImGui::Checkbox("Show Background", &settings.showEnvironmentBackground);
        ImGui::SliderFloat("Background Intensity", &settings.backgroundIntensity, 0.0f, 5.0f, "%.2f");
        ImGui::ColorEdit3("Sky Color", &settings.skyColor.x);
        ImGui::ColorEdit3("Ground Color", &settings.groundColor.x);

        ImGui::Separator();
        ImGui::TextUnformatted("Post Process");
        ImGui::SliderFloat("Exposure", &settings.exposure, 0.1f, 5.0f, "%.2f");

        ImGui::Separator();
        ImGui::TextUnformatted("Scene");
        ImGui::Checkbox("Rotate Object", &settings.rotateObject);
        ImGui::TextUnformatted("Camera: WASD, Space/Ctrl, RMB drag");
        ImGui::End();

        ImGui::SetNextWindowPos(ImVec2(372.0f, 16.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(220.0f, 0.0f), ImGuiCond_FirstUseEver);
        ImGui::Begin("Resource Preview");
        const Renderer::PreviewResources& preview = renderer.previewResources();
        ImGui::SliderInt("Cube Face", &settings.previewCubeFace, 0, 5);
        ImGui::SliderFloat("Prefilter Mip", &settings.previewPrefilterMip, 0.0f, 9.0f, "%.1f");
        if (preview.brdfLut)
        {
            ImGui::TextUnformatted("BRDF LUT");
            ImGui::Image(reinterpret_cast<ImTextureID>(preview.brdfLut), ImVec2(128.0f, 128.0f));
        }
        if (preview.irradiance)
        {
            ImGui::TextUnformatted("Irradiance");
            ImGui::Image(reinterpret_cast<ImTextureID>(preview.irradiance), ImVec2(128.0f, 128.0f));
        }
        if (preview.prefiltered)
        {
            ImGui::TextUnformatted("Prefiltered");
            ImGui::Image(reinterpret_cast<ImTextureID>(preview.prefiltered), ImVec2(128.0f, 128.0f));
        }
        ImGui::End();
    }

    void ImGuiLayer::endFrame()
    {
        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }
}
