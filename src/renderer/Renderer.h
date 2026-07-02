#pragma once

#include "core/MathTypes.h"
#include "debug/DebugSettings.h"
#include "renderer/DeviceResources.h"
#include "renderer/Material.h"
#include "renderer/Texture.h"
#include "scene/Camera.h"

#include <d3d11.h>
#include <filesystem>
#include <vector>
#include <wrl/client.h>

namespace dxsv
{
    class Renderer
    {
    public:
        void initialize(DeviceResources* deviceResources);
        void resize();
        void applyDebugSettings(const DebugSettings& settings);
        void render(float totalSeconds, const Camera& camera, const DebugSettings& settings);

        struct PreviewResources
        {
            ID3D11ShaderResourceView* brdfLut = nullptr;
            ID3D11ShaderResourceView* irradiance = nullptr;
            ID3D11ShaderResourceView* prefiltered = nullptr;
        };

        const PreviewResources& previewResources() const { return m_previewResources; }

    private:
        struct ObjectConstants
        {
            Matrix4 worldViewProjection;
            Matrix4 world;
            Vector3 cameraPosition;
            float padding = 0.0f;
        };

        struct CompositeConstants
        {
            Matrix4 inverseViewProjection;
            Vector3 cameraPosition;
            float exposure = 1.15f;
            float backgroundIntensity = 1.0f;
            float showEnvironmentBackground = 1.0f;
            float padding0 = 0.0f;
            float padding = 0.0f;
        };

        struct LightConstants
        {
            Vector3 direction{ -0.35f, 1.0f, -0.25f };
            float intensity = 4.0f;
            Vector3 color{ 1.0f, 1.0f, 1.0f };
            float diffuseIblStrength = 1.0f;
            Vector3 skyColor{ 0.38f, 0.48f, 0.62f };
            float iblEnabled = 1.0f;
            Vector3 groundColor{ 0.035f, 0.032f, 0.028f };
            float environmentMipCount = 1.0f;
            float specularIblStrength = 1.0f;
            Vector3 padding{ 0.0f, 0.0f, 0.0f };
        };

        struct EnvironmentPrefilterConstants
        {
            unsigned int faceIndex = 0;
            float roughness = 0.0f;
            Vector2 padding{ 0.0f, 0.0f };
        };

        struct EnvironmentConvertConstants
        {
            unsigned int faceIndex = 0;
            Vector3 padding{ 0.0f, 0.0f, 0.0f };
        };

        void createMeshResources();
        void createShaders();
        void createHdrRenderTarget();
        void createSamplerState();
        void createBrdfLut();
        void createMaterialResources();
        void loadMaterialTextures();
        void loadEnvironmentEquirectTexture();
        void createEnvironmentCubeResources();
        void renderEnvironmentCube();
        void createPrefilteredEnvironmentResources();
        void renderPrefilteredEnvironment();
        void createIrradianceEnvironmentResources();
        void renderIrradianceEnvironment();
        void createResourcePreviewResources();
        void renderResourcePreviews(const DebugSettings& settings);
        void renderCubePreview(
            ID3D11ShaderResourceView* cubeShaderResourceView,
            ID3D11RenderTargetView* previewRenderTargetView,
            unsigned int faceIndex,
            float mipLevel);
        bool loadCubeCache(const std::filesystem::path& path, ID3D11ShaderResourceView** shaderResourceView);
        void saveCubeCache(ID3D11Resource* resource, const std::filesystem::path& path);
        void renderScene(float totalSeconds, const Camera& camera, const DebugSettings& settings);
        void renderFinalComposite(const Camera& camera, const DebugSettings& settings);
        void updateObjectConstants(float totalSeconds, const Camera& camera, const DebugSettings& settings);
        void updateMaterialConstants();
        void updateCompositeConstants(const Camera& camera, const DebugSettings& settings);
        void updateLightConstants(const DebugSettings& settings);
        std::filesystem::path shaderPath(const wchar_t* fileName) const;
        std::filesystem::path texturePath(const wchar_t* fileName) const;
        std::filesystem::path textureCachePath(const wchar_t* fileName) const;

        DeviceResources* m_deviceResources = nullptr;
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_indexBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_objectConstantBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_materialConstantBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_compositeConstantBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_lightConstantBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_environmentPrefilterConstantBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_environmentConvertConstantBuffer;
        Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
        Microsoft::WRL::ComPtr<ID3D11VertexShader> m_compositeVertexShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> m_compositePixelShader;
        Microsoft::WRL::ComPtr<ID3D11VertexShader> m_brdfLutVertexShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> m_brdfLutPixelShader;
        Microsoft::WRL::ComPtr<ID3D11VertexShader> m_environmentPrefilterVertexShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> m_environmentPrefilterPixelShader;
        Microsoft::WRL::ComPtr<ID3D11VertexShader> m_environmentConvertVertexShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> m_environmentConvertPixelShader;
        Microsoft::WRL::ComPtr<ID3D11VertexShader> m_environmentIrradianceVertexShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> m_environmentIrradiancePixelShader;
        Microsoft::WRL::ComPtr<ID3D11VertexShader> m_cubePreviewVertexShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> m_cubePreviewPixelShader;
        Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
        Microsoft::WRL::ComPtr<ID3D11SamplerState> m_linearSampler;
        Microsoft::WRL::ComPtr<ID3D11Texture2D> m_hdrTexture;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_hdrRenderTargetView;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_hdrShaderResourceView;
        Microsoft::WRL::ComPtr<ID3D11Texture2D> m_brdfLutTexture;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_brdfLutRenderTargetView;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_brdfLutShaderResourceView;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_environmentEquirectShaderResourceView;
        Microsoft::WRL::ComPtr<ID3D11Texture2D> m_environmentCubeTexture;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_environmentCubeShaderResourceView;
        std::vector<Microsoft::WRL::ComPtr<ID3D11RenderTargetView>> m_environmentCubeRenderTargetViews;
        Microsoft::WRL::ComPtr<ID3D11Texture2D> m_prefilteredEnvironmentTexture;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_prefilteredEnvironmentShaderResourceView;
        std::vector<Microsoft::WRL::ComPtr<ID3D11RenderTargetView>> m_prefilteredEnvironmentRenderTargetViews;
        Microsoft::WRL::ComPtr<ID3D11Texture2D> m_irradianceEnvironmentTexture;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_irradianceEnvironmentShaderResourceView;
        std::vector<Microsoft::WRL::ComPtr<ID3D11RenderTargetView>> m_irradianceEnvironmentRenderTargetViews;
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_cubePreviewConstantBuffer;
        Microsoft::WRL::ComPtr<ID3D11Texture2D> m_irradiancePreviewTexture;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_irradiancePreviewRenderTargetView;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_irradiancePreviewShaderResourceView;
        Microsoft::WRL::ComPtr<ID3D11Texture2D> m_prefilteredPreviewTexture;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_prefilteredPreviewRenderTargetView;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_prefilteredPreviewShaderResourceView;
        PreviewResources m_previewResources;
        Material m_material;
        TextureMaterialSet m_activeTextureMaterialSet = TextureMaterialSet::Silver;
        UINT m_indexCount = 0;
    };
}
