#include "renderer/Renderer.h"

#include "core/Logger.h"
#include "renderer/Mesh.h"

#include <DirectXTex.h>
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <d3dcompiler.h>
#include <filesystem>
#include <string>
#include <vector>

using namespace DirectX;

namespace dxsv
{
    namespace
    {
        constexpr unsigned int kBrdfLutSize = 256;
        constexpr unsigned int kResourcePreviewSize = 128;
        constexpr unsigned int kEnvironmentCubeFaceSize = 2048;
        constexpr float kEnvironmentCubeMipCount = 12.0f;
        constexpr unsigned int kPrefilteredEnvironmentCubeFaceSize = 512;
        constexpr float kPrefilteredEnvironmentCubeMipCount = 10.0f;
        constexpr unsigned int kIrradianceEnvironmentCubeFaceSize = 64;
        constexpr const wchar_t* kEnvironmentHdrFile = L"environment_8k.hdr";
        constexpr const wchar_t* kEnvironmentCubeCacheFile = L"environment_cube_cache.dds";
        constexpr const wchar_t* kPrefilteredEnvironmentCacheFile = L"environment_prefiltered_cache.dds";
        constexpr const wchar_t* kIrradianceEnvironmentCacheFile = L"environment_irradiance_cache.dds";

        struct CubePreviewConstants
        {
            unsigned int faceIndex = 0;
            float mipLevel = 0.0f;
            Vector2 padding{ 0.0f, 0.0f };
        };

        class ScopedTimer
        {
        public:
            explicit ScopedTimer(std::wstring name)
                : m_name(std::move(name))
                , m_start(std::chrono::steady_clock::now())
            {
            }

            ~ScopedTimer()
            {
                const auto end = std::chrono::steady_clock::now();
                const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - m_start).count();
                Logger::info(m_name + L": " + std::to_wstring(milliseconds) + L" ms");
            }

        private:
            std::wstring m_name;
            std::chrono::steady_clock::time_point m_start;
        };

    }

    void Renderer::initialize(DeviceResources* deviceResources)
    {
        ScopedTimer timer(L"Renderer initialize");
        m_deviceResources = deviceResources;
        createMeshResources();
        createShaders();
        createHdrRenderTarget();
        createSamplerState();
        createBrdfLut();
        createMaterialResources();
        createResourcePreviewResources();
    }

    void Renderer::resize()
    {
        createHdrRenderTarget();
    }

    void Renderer::applyDebugSettings(const DebugSettings& settings)
    {
        if (settings.textureMaterialSet != m_activeTextureMaterialSet)
        {
            m_activeTextureMaterialSet = settings.textureMaterialSet;
            loadMaterialTextures();
        }

        m_material.constants.baseColorFactor = Vector4{ settings.baseColor.x, settings.baseColor.y, settings.baseColor.z, 1.0f };
        m_material.constants.metallicFactor = settings.metallicFactor;
        m_material.constants.roughnessFactor = settings.roughnessFactor;
        m_material.constants.aoStrength = settings.aoStrength;
        m_material.constants.normalMapStrength = settings.normalMapping ? settings.normalMapStrength : 0.0f;
        m_material.constants.debugViewMode = static_cast<int>(settings.pbrDebugView);
        updateMaterialConstants();
    }

    void Renderer::render(float totalSeconds, const Camera& camera, const DebugSettings& settings)
    {
        applyDebugSettings(settings);
        renderScene(totalSeconds, camera, settings);
        renderFinalComposite(camera, settings);
        renderResourcePreviews(settings);
    }

    void Renderer::renderScene(float totalSeconds, const Camera& camera, const DebugSettings& settings)
    {
        updateObjectConstants(totalSeconds, camera, settings);

        ID3D11DeviceContext* context = m_deviceResources->context();
        ID3D11ShaderResourceView* nullSrvs[] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
        context->PSSetShaderResources(0, static_cast<UINT>(std::size(nullSrvs)), nullSrvs);

        const float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
        context->ClearRenderTargetView(m_hdrRenderTargetView.Get(), clearColor);
        context->ClearDepthStencilView(m_deviceResources->depthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
        ID3D11RenderTargetView* sceneTargets[] = { m_hdrRenderTargetView.Get() };
        context->OMSetRenderTargets(1, sceneTargets, m_deviceResources->depthStencilView());

        constexpr UINT stride = sizeof(Vertex);
        constexpr UINT offset = 0;

        ID3D11Buffer* vertexBuffers[] = { m_vertexBuffer.Get() };
        context->IASetVertexBuffers(0, 1, vertexBuffers, &stride, &offset);
        context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        context->IASetInputLayout(m_inputLayout.Get());
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        ID3D11Buffer* constantBuffers[] = { m_objectConstantBuffer.Get() };
        ID3D11Buffer* materialConstantBuffers[] = { m_materialConstantBuffer.Get() };
        ID3D11Buffer* lightConstantBuffers[] = { m_lightConstantBuffer.Get() };
        ID3D11ShaderResourceView* shaderResources[] = {
            m_material.albedo.shaderResourceView(),
            m_material.normal.shaderResourceView(),
            m_material.metallic.shaderResourceView(),
            m_material.roughness.shaderResourceView(),
            m_material.ao.shaderResourceView(),
            m_environmentCubeShaderResourceView.Get(),
            m_brdfLutShaderResourceView.Get(),
            m_prefilteredEnvironmentShaderResourceView.Get(),
            m_irradianceEnvironmentShaderResourceView.Get(),
        };
        ID3D11SamplerState* samplers[] = { m_linearSampler.Get() };
        D3D11_VIEWPORT viewport{};
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        viewport.Width = static_cast<float>(m_deviceResources->width());
        viewport.Height = static_cast<float>(m_deviceResources->height());
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        context->RSSetViewports(1, &viewport);
        context->VSSetConstantBuffers(0, 1, constantBuffers);
        context->PSSetConstantBuffers(0, 1, constantBuffers);
        context->PSSetConstantBuffers(1, 1, materialConstantBuffers);
        context->PSSetConstantBuffers(2, 1, lightConstantBuffers);
        context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
        context->PSSetShaderResources(0, static_cast<UINT>(std::size(shaderResources)), shaderResources);
        context->PSSetSamplers(0, 1, samplers);
        context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
        context->DrawIndexed(m_indexCount, 0, 0);
    }

    void Renderer::renderFinalComposite(const Camera& camera, const DebugSettings& settings)
    {
        ID3D11DeviceContext* context = m_deviceResources->context();
        updateCompositeConstants(camera, settings);
        m_deviceResources->setBackBufferRenderTarget();

        const float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        context->ClearRenderTargetView(m_deviceResources->renderTargetView(), clearColor);

        ID3D11ShaderResourceView* shaderResources[] = {
            m_hdrShaderResourceView.Get(),
            m_environmentCubeShaderResourceView.Get(),
        };
        ID3D11SamplerState* samplers[] = { m_linearSampler.Get() };
        ID3D11Buffer* compositeConstantBuffers[] = { m_compositeConstantBuffer.Get() };
        context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
        context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
        context->IASetInputLayout(nullptr);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->VSSetShader(m_compositeVertexShader.Get(), nullptr, 0);
        context->PSSetConstantBuffers(0, 1, compositeConstantBuffers);
        context->PSSetShaderResources(0, static_cast<UINT>(std::size(shaderResources)), shaderResources);
        context->PSSetSamplers(0, 1, samplers);
        context->PSSetShader(m_compositePixelShader.Get(), nullptr, 0);
        context->Draw(3, 0);

        ID3D11ShaderResourceView* nullSrvs[] = { nullptr, nullptr };
        context->PSSetShaderResources(0, static_cast<UINT>(std::size(nullSrvs)), nullSrvs);
    }

    void Renderer::createMeshResources()
    {
        const Mesh mesh = Mesh::createSphere(1.25f, 96, 48);
        m_indexCount = static_cast<UINT>(mesh.indices().size());

        D3D11_BUFFER_DESC vertexBufferDesc{};
        vertexBufferDesc.ByteWidth = static_cast<UINT>(mesh.vertices().size() * sizeof(Vertex));
        vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vertexData{};
        vertexData.pSysMem = mesh.vertices().data();

        throwIfFailed(
            m_deviceResources->device()->CreateBuffer(&vertexBufferDesc, &vertexData, m_vertexBuffer.GetAddressOf()),
            "CreateBuffer vertex buffer failed.");

        D3D11_BUFFER_DESC indexBufferDesc{};
        indexBufferDesc.ByteWidth = static_cast<UINT>(mesh.indices().size() * sizeof(unsigned int));
        indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA indexData{};
        indexData.pSysMem = mesh.indices().data();

        throwIfFailed(
            m_deviceResources->device()->CreateBuffer(&indexBufferDesc, &indexData, m_indexBuffer.GetAddressOf()),
            "CreateBuffer index buffer failed.");

        D3D11_BUFFER_DESC constantBufferDesc{};
        constantBufferDesc.ByteWidth = sizeof(ObjectConstants);
        constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

        throwIfFailed(
            m_deviceResources->device()->CreateBuffer(&constantBufferDesc, nullptr, m_objectConstantBuffer.GetAddressOf()),
            "CreateBuffer constant buffer failed.");

        D3D11_BUFFER_DESC materialBufferDesc{};
        materialBufferDesc.ByteWidth = sizeof(Material::Constants);
        materialBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        materialBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

        throwIfFailed(
            m_deviceResources->device()->CreateBuffer(&materialBufferDesc, nullptr, m_materialConstantBuffer.GetAddressOf()),
            "CreateBuffer material constant buffer failed.");

        D3D11_BUFFER_DESC compositeBufferDesc{};
        compositeBufferDesc.ByteWidth = sizeof(CompositeConstants);
        compositeBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        compositeBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

        throwIfFailed(
            m_deviceResources->device()->CreateBuffer(&compositeBufferDesc, nullptr, m_compositeConstantBuffer.GetAddressOf()),
            "CreateBuffer composite constant buffer failed.");

        D3D11_BUFFER_DESC lightBufferDesc{};
        lightBufferDesc.ByteWidth = sizeof(LightConstants);
        lightBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        lightBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

        throwIfFailed(
            m_deviceResources->device()->CreateBuffer(&lightBufferDesc, nullptr, m_lightConstantBuffer.GetAddressOf()),
            "CreateBuffer light constant buffer failed.");

        D3D11_BUFFER_DESC prefilterBufferDesc{};
        prefilterBufferDesc.ByteWidth = sizeof(EnvironmentPrefilterConstants);
        prefilterBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        prefilterBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

        throwIfFailed(
            m_deviceResources->device()->CreateBuffer(
                &prefilterBufferDesc,
                nullptr,
                m_environmentPrefilterConstantBuffer.GetAddressOf()),
            "CreateBuffer environment prefilter constant buffer failed.");

        D3D11_BUFFER_DESC convertBufferDesc{};
        convertBufferDesc.ByteWidth = sizeof(EnvironmentConvertConstants);
        convertBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        convertBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

        throwIfFailed(
            m_deviceResources->device()->CreateBuffer(
                &convertBufferDesc,
                nullptr,
                m_environmentConvertConstantBuffer.GetAddressOf()),
            "CreateBuffer environment convert constant buffer failed.");

        D3D11_BUFFER_DESC cubePreviewBufferDesc{};
        cubePreviewBufferDesc.ByteWidth = sizeof(CubePreviewConstants);
        cubePreviewBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        cubePreviewBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

        throwIfFailed(
            m_deviceResources->device()->CreateBuffer(
                &cubePreviewBufferDesc,
                nullptr,
                m_cubePreviewConstantBuffer.GetAddressOf()),
            "CreateBuffer cube preview constant buffer failed.");
    }

    void Renderer::createShaders()
    {
        Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> compositeVertexShaderBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> compositePixelShaderBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> brdfLutVertexShaderBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> brdfLutPixelShaderBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> cubePreviewVertexShaderBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> cubePreviewPixelShaderBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> environmentPrefilterVertexShaderBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> environmentPrefilterPixelShaderBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> environmentConvertVertexShaderBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> environmentConvertPixelShaderBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> environmentIrradianceVertexShaderBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> environmentIrradiancePixelShaderBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

        const std::filesystem::path path = shaderPath(L"PBR.hlsl");
        const std::filesystem::path compositePath = shaderPath(L"FinalComposite.hlsl");
        const std::filesystem::path brdfLutPath = shaderPath(L"BrdfLut.hlsl");
        const std::filesystem::path cubePreviewPath = shaderPath(L"CubePreview.hlsl");
        const std::filesystem::path environmentPrefilterPath = shaderPath(L"EnvironmentPrefilter.hlsl");
        const std::filesystem::path environmentConvertPath = shaderPath(L"EnvironmentConvert.hlsl");
        const std::filesystem::path environmentIrradiancePath = shaderPath(L"EnvironmentIrradiance.hlsl");

        UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(_DEBUG)
        compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

        HRESULT hr = D3DCompileFromFile(
            path.c_str(),
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "VSMain",
            "vs_5_0",
            compileFlags,
            0,
            vertexShaderBlob.GetAddressOf(),
            errorBlob.GetAddressOf());
        throwIfFailed(hr, "Vertex shader compile failed.");

        errorBlob.Reset();
        hr = D3DCompileFromFile(
            path.c_str(),
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "PSMain",
            "ps_5_0",
            compileFlags,
            0,
            pixelShaderBlob.GetAddressOf(),
            errorBlob.GetAddressOf());
        throwIfFailed(hr, "Pixel shader compile failed.");

        throwIfFailed(
            m_deviceResources->device()->CreateVertexShader(
                vertexShaderBlob->GetBufferPointer(),
                vertexShaderBlob->GetBufferSize(),
                nullptr,
                m_vertexShader.GetAddressOf()),
            "CreateVertexShader failed.");

        throwIfFailed(
            m_deviceResources->device()->CreatePixelShader(
                pixelShaderBlob->GetBufferPointer(),
                pixelShaderBlob->GetBufferSize(),
                nullptr,
                m_pixelShader.GetAddressOf()),
            "CreatePixelShader failed.");

        const D3D11_INPUT_ELEMENT_DESC inputElements[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, normal), D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, tangent), D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(Vertex, color), D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, texcoord), D3D11_INPUT_PER_VERTEX_DATA, 0},
        };

        throwIfFailed(
            m_deviceResources->device()->CreateInputLayout(
                inputElements,
                static_cast<UINT>(std::size(inputElements)),
                vertexShaderBlob->GetBufferPointer(),
                vertexShaderBlob->GetBufferSize(),
                m_inputLayout.GetAddressOf()),
            "CreateInputLayout failed.");

        errorBlob.Reset();
        hr = D3DCompileFromFile(
            compositePath.c_str(),
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "VSMain",
            "vs_5_0",
            compileFlags,
            0,
            compositeVertexShaderBlob.GetAddressOf(),
            errorBlob.GetAddressOf());
        throwIfFailed(hr, "Composite vertex shader compile failed.");

        errorBlob.Reset();
        hr = D3DCompileFromFile(
            compositePath.c_str(),
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "PSMain",
            "ps_5_0",
            compileFlags,
            0,
            compositePixelShaderBlob.GetAddressOf(),
            errorBlob.GetAddressOf());
        throwIfFailed(hr, "Composite pixel shader compile failed.");

        throwIfFailed(
            m_deviceResources->device()->CreateVertexShader(
                compositeVertexShaderBlob->GetBufferPointer(),
                compositeVertexShaderBlob->GetBufferSize(),
                nullptr,
                m_compositeVertexShader.GetAddressOf()),
            "CreateVertexShader composite failed.");

        throwIfFailed(
            m_deviceResources->device()->CreatePixelShader(
                compositePixelShaderBlob->GetBufferPointer(),
                compositePixelShaderBlob->GetBufferSize(),
                nullptr,
                m_compositePixelShader.GetAddressOf()),
            "CreatePixelShader composite failed.");

        errorBlob.Reset();
        hr = D3DCompileFromFile(
            brdfLutPath.c_str(),
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "VSMain",
            "vs_5_0",
            compileFlags,
            0,
            brdfLutVertexShaderBlob.GetAddressOf(),
            errorBlob.GetAddressOf());
        throwIfFailed(hr, "BRDF LUT vertex shader compile failed.");

        errorBlob.Reset();
        hr = D3DCompileFromFile(
            brdfLutPath.c_str(),
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "PSMain",
            "ps_5_0",
            compileFlags,
            0,
            brdfLutPixelShaderBlob.GetAddressOf(),
            errorBlob.GetAddressOf());
        throwIfFailed(hr, "BRDF LUT pixel shader compile failed.");

        throwIfFailed(
            m_deviceResources->device()->CreateVertexShader(
                brdfLutVertexShaderBlob->GetBufferPointer(),
                brdfLutVertexShaderBlob->GetBufferSize(),
                nullptr,
                m_brdfLutVertexShader.GetAddressOf()),
            "CreateVertexShader BRDF LUT failed.");

        throwIfFailed(
            m_deviceResources->device()->CreatePixelShader(
                brdfLutPixelShaderBlob->GetBufferPointer(),
                brdfLutPixelShaderBlob->GetBufferSize(),
                nullptr,
                m_brdfLutPixelShader.GetAddressOf()),
            "CreatePixelShader BRDF LUT failed.");

        errorBlob.Reset();
        hr = D3DCompileFromFile(
            cubePreviewPath.c_str(),
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "VSMain",
            "vs_5_0",
            compileFlags,
            0,
            cubePreviewVertexShaderBlob.GetAddressOf(),
            errorBlob.GetAddressOf());
        throwIfFailed(hr, "Cube preview vertex shader compile failed.");

        errorBlob.Reset();
        hr = D3DCompileFromFile(
            cubePreviewPath.c_str(),
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "PSMain",
            "ps_5_0",
            compileFlags,
            0,
            cubePreviewPixelShaderBlob.GetAddressOf(),
            errorBlob.GetAddressOf());
        throwIfFailed(hr, "Cube preview pixel shader compile failed.");

        throwIfFailed(
            m_deviceResources->device()->CreateVertexShader(
                cubePreviewVertexShaderBlob->GetBufferPointer(),
                cubePreviewVertexShaderBlob->GetBufferSize(),
                nullptr,
                m_cubePreviewVertexShader.GetAddressOf()),
            "CreateVertexShader cube preview failed.");

        throwIfFailed(
            m_deviceResources->device()->CreatePixelShader(
                cubePreviewPixelShaderBlob->GetBufferPointer(),
                cubePreviewPixelShaderBlob->GetBufferSize(),
                nullptr,
                m_cubePreviewPixelShader.GetAddressOf()),
            "CreatePixelShader cube preview failed.");

        errorBlob.Reset();
        hr = D3DCompileFromFile(
            environmentPrefilterPath.c_str(),
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "VSMain",
            "vs_5_0",
            compileFlags,
            0,
            environmentPrefilterVertexShaderBlob.GetAddressOf(),
            errorBlob.GetAddressOf());
        throwIfFailed(hr, "Environment prefilter vertex shader compile failed.");

        errorBlob.Reset();
        hr = D3DCompileFromFile(
            environmentPrefilterPath.c_str(),
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "PSMain",
            "ps_5_0",
            compileFlags,
            0,
            environmentPrefilterPixelShaderBlob.GetAddressOf(),
            errorBlob.GetAddressOf());
        throwIfFailed(hr, "Environment prefilter pixel shader compile failed.");

        throwIfFailed(
            m_deviceResources->device()->CreateVertexShader(
                environmentPrefilterVertexShaderBlob->GetBufferPointer(),
                environmentPrefilterVertexShaderBlob->GetBufferSize(),
                nullptr,
                m_environmentPrefilterVertexShader.GetAddressOf()),
            "CreateVertexShader environment prefilter failed.");

        throwIfFailed(
            m_deviceResources->device()->CreatePixelShader(
                environmentPrefilterPixelShaderBlob->GetBufferPointer(),
                environmentPrefilterPixelShaderBlob->GetBufferSize(),
                nullptr,
                m_environmentPrefilterPixelShader.GetAddressOf()),
            "CreatePixelShader environment prefilter failed.");

        errorBlob.Reset();
        hr = D3DCompileFromFile(
            environmentConvertPath.c_str(),
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "VSMain",
            "vs_5_0",
            compileFlags,
            0,
            environmentConvertVertexShaderBlob.GetAddressOf(),
            errorBlob.GetAddressOf());
        throwIfFailed(hr, "Environment convert vertex shader compile failed.");

        errorBlob.Reset();
        hr = D3DCompileFromFile(
            environmentConvertPath.c_str(),
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "PSMain",
            "ps_5_0",
            compileFlags,
            0,
            environmentConvertPixelShaderBlob.GetAddressOf(),
            errorBlob.GetAddressOf());
        throwIfFailed(hr, "Environment convert pixel shader compile failed.");

        throwIfFailed(
            m_deviceResources->device()->CreateVertexShader(
                environmentConvertVertexShaderBlob->GetBufferPointer(),
                environmentConvertVertexShaderBlob->GetBufferSize(),
                nullptr,
                m_environmentConvertVertexShader.GetAddressOf()),
            "CreateVertexShader environment convert failed.");

        throwIfFailed(
            m_deviceResources->device()->CreatePixelShader(
                environmentConvertPixelShaderBlob->GetBufferPointer(),
                environmentConvertPixelShaderBlob->GetBufferSize(),
                nullptr,
                m_environmentConvertPixelShader.GetAddressOf()),
            "CreatePixelShader environment convert failed.");

        errorBlob.Reset();
        hr = D3DCompileFromFile(
            environmentIrradiancePath.c_str(),
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "VSMain",
            "vs_5_0",
            compileFlags,
            0,
            environmentIrradianceVertexShaderBlob.GetAddressOf(),
            errorBlob.GetAddressOf());
        throwIfFailed(hr, "Environment irradiance vertex shader compile failed.");

        errorBlob.Reset();
        hr = D3DCompileFromFile(
            environmentIrradiancePath.c_str(),
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "PSMain",
            "ps_5_0",
            compileFlags,
            0,
            environmentIrradiancePixelShaderBlob.GetAddressOf(),
            errorBlob.GetAddressOf());
        throwIfFailed(hr, "Environment irradiance pixel shader compile failed.");

        throwIfFailed(
            m_deviceResources->device()->CreateVertexShader(
                environmentIrradianceVertexShaderBlob->GetBufferPointer(),
                environmentIrradianceVertexShaderBlob->GetBufferSize(),
                nullptr,
                m_environmentIrradianceVertexShader.GetAddressOf()),
            "CreateVertexShader environment irradiance failed.");

        throwIfFailed(
            m_deviceResources->device()->CreatePixelShader(
                environmentIrradiancePixelShaderBlob->GetBufferPointer(),
                environmentIrradiancePixelShaderBlob->GetBufferSize(),
                nullptr,
                m_environmentIrradiancePixelShader.GetAddressOf()),
            "CreatePixelShader environment irradiance failed.");
    }

    void Renderer::createHdrRenderTarget()
    {
        m_hdrRenderTargetView.Reset();
        m_hdrShaderResourceView.Reset();
        m_hdrTexture.Reset();

        D3D11_TEXTURE2D_DESC textureDesc{};
        textureDesc.Width = static_cast<UINT>(m_deviceResources->width());
        textureDesc.Height = static_cast<UINT>(m_deviceResources->height());
        textureDesc.MipLevels = 1;
        textureDesc.ArraySize = 1;
        textureDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

        throwIfFailed(
            m_deviceResources->device()->CreateTexture2D(&textureDesc, nullptr, m_hdrTexture.GetAddressOf()),
            "CreateTexture2D HDR render target failed.");

        throwIfFailed(
            m_deviceResources->device()->CreateRenderTargetView(m_hdrTexture.Get(), nullptr, m_hdrRenderTargetView.GetAddressOf()),
            "CreateRenderTargetView HDR failed.");

        throwIfFailed(
            m_deviceResources->device()->CreateShaderResourceView(m_hdrTexture.Get(), nullptr, m_hdrShaderResourceView.GetAddressOf()),
            "CreateShaderResourceView HDR failed.");
    }

    void Renderer::createSamplerState()
    {
        D3D11_SAMPLER_DESC samplerDesc{};
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        samplerDesc.MinLOD = 0.0f;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

        throwIfFailed(
            m_deviceResources->device()->CreateSamplerState(&samplerDesc, m_linearSampler.GetAddressOf()),
            "CreateSamplerState failed.");
    }

    void Renderer::createBrdfLut()
    {
        ScopedTimer timer(L"GPU BRDF LUT integration");

        D3D11_TEXTURE2D_DESC textureDesc{};
        textureDesc.Width = kBrdfLutSize;
        textureDesc.Height = kBrdfLutSize;
        textureDesc.MipLevels = 1;
        textureDesc.ArraySize = 1;
        textureDesc.Format = DXGI_FORMAT_R16G16_FLOAT;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

        throwIfFailed(
            m_deviceResources->device()->CreateTexture2D(&textureDesc, nullptr, m_brdfLutTexture.GetAddressOf()),
            "CreateTexture2D BRDF LUT failed.");

        throwIfFailed(
            m_deviceResources->device()->CreateRenderTargetView(
                m_brdfLutTexture.Get(),
                nullptr,
                m_brdfLutRenderTargetView.GetAddressOf()),
            "CreateRenderTargetView BRDF LUT failed.");

        throwIfFailed(
            m_deviceResources->device()->CreateShaderResourceView(m_brdfLutTexture.Get(), nullptr, m_brdfLutShaderResourceView.GetAddressOf()),
            "CreateShaderResourceView BRDF LUT failed.");

        ID3D11DeviceContext* context = m_deviceResources->context();
        ID3D11RenderTargetView* renderTargets[] = { m_brdfLutRenderTargetView.Get() };

        D3D11_VIEWPORT viewport{};
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        viewport.Width = static_cast<float>(kBrdfLutSize);
        viewport.Height = static_cast<float>(kBrdfLutSize);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        context->RSSetViewports(1, &viewport);

        const float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        context->ClearRenderTargetView(renderTargets[0], clearColor);
        context->OMSetRenderTargets(1, renderTargets, nullptr);

        context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
        context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
        context->IASetInputLayout(nullptr);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->VSSetShader(m_brdfLutVertexShader.Get(), nullptr, 0);
        context->PSSetShader(m_brdfLutPixelShader.Get(), nullptr, 0);
        context->Draw(3, 0);

        ID3D11RenderTargetView* nullRenderTarget[] = { nullptr };
        context->OMSetRenderTargets(1, nullRenderTarget, nullptr);

        viewport.Width = static_cast<float>(m_deviceResources->width());
        viewport.Height = static_cast<float>(m_deviceResources->height());
        context->RSSetViewports(1, &viewport);
    }

    void Renderer::createMaterialResources()
    {
        ScopedTimer timer(L"Material and environment resources");
        loadMaterialTextures();
        createEnvironmentCubeResources();
        createPrefilteredEnvironmentResources();
        createIrradianceEnvironmentResources();
        updateMaterialConstants();
    }

    void Renderer::loadMaterialTextures()
    {
        ScopedTimer timer(L"Load material textures");
        ID3D11ShaderResourceView* nullSrvs[] = { nullptr, nullptr, nullptr, nullptr, nullptr };
        m_deviceResources->context()->PSSetShaderResources(0, static_cast<UINT>(std::size(nullSrvs)), nullSrvs);

        PbrTexturePaths paths{};
        switch (m_activeTextureMaterialSet)
        {
        case TextureMaterialSet::LightGold:
            paths.albedo = texturePath(L"light-gold-bl/lightgold_albedo.png");
            paths.normal = texturePath(L"light-gold-bl/lightgold_normal-ogl.png");
            paths.metallic = texturePath(L"light-gold-bl/lightgold_metallic.png");
            paths.roughness = texturePath(L"light-gold-bl/lightgold_roughness.png");
            paths.ao = texturePath(L"light-gold-bl/lightgold_ao.png");
            break;
        case TextureMaterialSet::Bronze:
            paths.albedo = texturePath(L"bronze-bl/bronze_albedo.png");
            paths.normal = texturePath(L"bronze-bl/bronze_normal-ogl.png");
            paths.metallic = texturePath(L"bronze-bl/bronze_metallic.png");
            paths.roughness = texturePath(L"bronze-bl/bronze_roughness.png");
            paths.ao = texturePath(L"bronze-bl/bronze_ao.png");
            break;
        case TextureMaterialSet::Silver:
        default:
            paths.albedo = texturePath(L"silver-bl/silver_albedo.png");
            paths.normal = texturePath(L"silver-bl/silver_normal-ogl.png");
            paths.metallic = texturePath(L"silver-bl/silver_metallic.png");
            paths.roughness = texturePath(L"silver-bl/silver_roughness.png");
            paths.ao = texturePath(L"silver-bl/silver_ao.png");
            break;
        }

        m_material.loadPbrTextures(m_deviceResources->device(), paths);
    }

    void Renderer::loadEnvironmentEquirectTexture()
    {
        ScopedTimer timer(L"Load HDR environment");
        DirectX::ScratchImage image;
        DirectX::TexMetadata metadata{};
        throwIfFailed(
            DirectX::LoadFromHDRFile(texturePath(kEnvironmentHdrFile).c_str(), &metadata, image),
            "LoadFromHDRFile environment failed.");

        throwIfFailed(
            DirectX::CreateShaderResourceView(
                m_deviceResources->device(),
                image.GetImages(),
                image.GetImageCount(),
                metadata,
                m_environmentEquirectShaderResourceView.GetAddressOf()),
            "CreateShaderResourceView environment equirect failed.");
    }

    void Renderer::createEnvironmentCubeResources()
    {
        ScopedTimer timer(L"Create/load environment cubemap");
        if (loadCubeCache(textureCachePath(kEnvironmentCubeCacheFile), m_environmentCubeShaderResourceView.GetAddressOf()))
        {
            Logger::info(L"Loaded environment cubemap DDS cache.");
            return;
        }

        loadEnvironmentEquirectTexture();

        ID3D11Device* device = m_deviceResources->device();

        D3D11_TEXTURE2D_DESC textureDesc{};
        textureDesc.Width = kEnvironmentCubeFaceSize;
        textureDesc.Height = kEnvironmentCubeFaceSize;
        textureDesc.MipLevels = static_cast<UINT>(kEnvironmentCubeMipCount);
        textureDesc.ArraySize = 6;
        textureDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        textureDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE | D3D11_RESOURCE_MISC_GENERATE_MIPS;

        throwIfFailed(
            device->CreateTexture2D(&textureDesc, nullptr, m_environmentCubeTexture.GetAddressOf()),
            "CreateTexture2D environment cubemap failed.");

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = textureDesc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.TextureCube.MipLevels = textureDesc.MipLevels;

        throwIfFailed(
            device->CreateShaderResourceView(
                m_environmentCubeTexture.Get(),
                &srvDesc,
                m_environmentCubeShaderResourceView.GetAddressOf()),
            "CreateShaderResourceView environment cubemap failed.");

        m_environmentCubeRenderTargetViews.clear();
        m_environmentCubeRenderTargetViews.resize(6);

        for (UINT face = 0; face < 6; ++face)
        {
            D3D11_RENDER_TARGET_VIEW_DESC rtvDesc{};
            rtvDesc.Format = textureDesc.Format;
            rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
            rtvDesc.Texture2DArray.MipSlice = 0;
            rtvDesc.Texture2DArray.FirstArraySlice = face;
            rtvDesc.Texture2DArray.ArraySize = 1;

            throwIfFailed(
                device->CreateRenderTargetView(
                    m_environmentCubeTexture.Get(),
                    &rtvDesc,
                    m_environmentCubeRenderTargetViews[face].GetAddressOf()),
                "CreateRenderTargetView environment cubemap failed.");
        }

        renderEnvironmentCube();
        saveCubeCache(m_environmentCubeTexture.Get(), textureCachePath(kEnvironmentCubeCacheFile));
    }

    void Renderer::renderEnvironmentCube()
    {
        ScopedTimer timer(L"GPU equirect to cubemap");
        ID3D11DeviceContext* context = m_deviceResources->context();

        ID3D11RenderTargetView* nullRenderTarget[] = { nullptr };
        ID3D11ShaderResourceView* nullSrvs[] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
        context->PSSetShaderResources(0, static_cast<UINT>(std::size(nullSrvs)), nullSrvs);

        D3D11_VIEWPORT viewport{};
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        viewport.Width = static_cast<float>(kEnvironmentCubeFaceSize);
        viewport.Height = static_cast<float>(kEnvironmentCubeFaceSize);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        context->RSSetViewports(1, &viewport);

        ID3D11ShaderResourceView* shaderResources[] = { m_environmentEquirectShaderResourceView.Get() };
        ID3D11SamplerState* samplers[] = { m_linearSampler.Get() };
        ID3D11Buffer* constantBuffers[] = { m_environmentConvertConstantBuffer.Get() };

        context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
        context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
        context->IASetInputLayout(nullptr);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->VSSetShader(m_environmentConvertVertexShader.Get(), nullptr, 0);
        context->PSSetShader(m_environmentConvertPixelShader.Get(), nullptr, 0);
        context->PSSetShaderResources(0, 1, shaderResources);
        context->PSSetSamplers(0, 1, samplers);
        context->PSSetConstantBuffers(0, 1, constantBuffers);

        for (UINT face = 0; face < 6; ++face)
        {
            ID3D11RenderTargetView* renderTargets[] = { m_environmentCubeRenderTargetViews[face].Get() };
            const float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
            context->ClearRenderTargetView(renderTargets[0], clearColor);
            context->OMSetRenderTargets(1, renderTargets, nullptr);

            EnvironmentConvertConstants constants{};
            constants.faceIndex = face;
            context->UpdateSubresource(m_environmentConvertConstantBuffer.Get(), 0, nullptr, &constants, 0, 0);
            context->Draw(3, 0);
        }

        context->OMSetRenderTargets(1, nullRenderTarget, nullptr);
        context->PSSetShaderResources(0, static_cast<UINT>(std::size(nullSrvs)), nullSrvs);
        context->GenerateMips(m_environmentCubeShaderResourceView.Get());

        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        viewport.Width = static_cast<float>(m_deviceResources->width());
        viewport.Height = static_cast<float>(m_deviceResources->height());
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        context->RSSetViewports(1, &viewport);
    }

    void Renderer::createPrefilteredEnvironmentResources()
    {
        ScopedTimer timer(L"Create/load prefiltered environment");
        if (loadCubeCache(textureCachePath(kPrefilteredEnvironmentCacheFile), m_prefilteredEnvironmentShaderResourceView.GetAddressOf()))
        {
            Logger::info(L"Loaded prefiltered environment DDS cache.");
            return;
        }

        ID3D11Device* device = m_deviceResources->device();

        D3D11_TEXTURE2D_DESC textureDesc{};
        textureDesc.Width = kPrefilteredEnvironmentCubeFaceSize;
        textureDesc.Height = kPrefilteredEnvironmentCubeFaceSize;
        textureDesc.MipLevels = static_cast<UINT>(kPrefilteredEnvironmentCubeMipCount);
        textureDesc.ArraySize = 6;
        textureDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        textureDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

        throwIfFailed(
            device->CreateTexture2D(&textureDesc, nullptr, m_prefilteredEnvironmentTexture.GetAddressOf()),
            "CreateTexture2D GPU prefiltered environment failed.");

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = textureDesc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.TextureCube.MipLevels = textureDesc.MipLevels;

        throwIfFailed(
            device->CreateShaderResourceView(
                m_prefilteredEnvironmentTexture.Get(),
                &srvDesc,
                m_prefilteredEnvironmentShaderResourceView.GetAddressOf()),
            "CreateShaderResourceView GPU prefiltered environment failed.");

        m_prefilteredEnvironmentRenderTargetViews.clear();
        m_prefilteredEnvironmentRenderTargetViews.resize(static_cast<size_t>(textureDesc.MipLevels) * 6);

        for (UINT face = 0; face < 6; ++face)
        {
            for (UINT mip = 0; mip < textureDesc.MipLevels; ++mip)
            {
                D3D11_RENDER_TARGET_VIEW_DESC rtvDesc{};
                rtvDesc.Format = textureDesc.Format;
                rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                rtvDesc.Texture2DArray.MipSlice = mip;
                rtvDesc.Texture2DArray.FirstArraySlice = face;
                rtvDesc.Texture2DArray.ArraySize = 1;

                const size_t index = static_cast<size_t>(face) * textureDesc.MipLevels + mip;
                throwIfFailed(
                    device->CreateRenderTargetView(
                        m_prefilteredEnvironmentTexture.Get(),
                        &rtvDesc,
                        m_prefilteredEnvironmentRenderTargetViews[index].GetAddressOf()),
                    "CreateRenderTargetView GPU prefiltered environment failed.");
            }
        }

        renderPrefilteredEnvironment();
        saveCubeCache(m_prefilteredEnvironmentTexture.Get(), textureCachePath(kPrefilteredEnvironmentCacheFile));
    }

    void Renderer::renderPrefilteredEnvironment()
    {
        ScopedTimer timer(L"GPU GGX environment prefilter");
        ID3D11DeviceContext* context = m_deviceResources->context();
        constexpr UINT mipCount = static_cast<UINT>(kPrefilteredEnvironmentCubeMipCount);

        ID3D11RenderTargetView* nullRenderTarget[] = { nullptr };
        ID3D11ShaderResourceView* nullSrvs[] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
        context->PSSetShaderResources(0, static_cast<UINT>(std::size(nullSrvs)), nullSrvs);

        ID3D11ShaderResourceView* shaderResources[] = { m_environmentCubeShaderResourceView.Get() };
        ID3D11SamplerState* samplers[] = { m_linearSampler.Get() };
        ID3D11Buffer* constantBuffers[] = { m_environmentPrefilterConstantBuffer.Get() };

        context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
        context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
        context->IASetInputLayout(nullptr);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->VSSetShader(m_environmentPrefilterVertexShader.Get(), nullptr, 0);
        context->PSSetShader(m_environmentPrefilterPixelShader.Get(), nullptr, 0);
        context->PSSetShaderResources(0, 1, shaderResources);
        context->PSSetSamplers(0, 1, samplers);
        context->PSSetConstantBuffers(0, 1, constantBuffers);

        for (UINT mip = 0; mip < mipCount; ++mip)
        {
            const UINT mipSize = (std::max)(1u, kPrefilteredEnvironmentCubeFaceSize >> mip);
            D3D11_VIEWPORT viewport{};
            viewport.TopLeftX = 0.0f;
            viewport.TopLeftY = 0.0f;
            viewport.Width = static_cast<float>(mipSize);
            viewport.Height = static_cast<float>(mipSize);
            viewport.MinDepth = 0.0f;
            viewport.MaxDepth = 1.0f;
            context->RSSetViewports(1, &viewport);

            for (UINT face = 0; face < 6; ++face)
            {
                const size_t rtvIndex = static_cast<size_t>(face) * mipCount + mip;
                ID3D11RenderTargetView* renderTargets[] = {
                    m_prefilteredEnvironmentRenderTargetViews[rtvIndex].Get(),
                };
                const float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
                context->ClearRenderTargetView(renderTargets[0], clearColor);
                context->OMSetRenderTargets(1, renderTargets, nullptr);

                EnvironmentPrefilterConstants constants{};
                constants.faceIndex = face;
                constants.roughness = static_cast<float>(mip) / static_cast<float>(mipCount - 1);
                context->UpdateSubresource(
                    m_environmentPrefilterConstantBuffer.Get(),
                    0,
                    nullptr,
                    &constants,
                    0,
                    0);

                context->Draw(3, 0);
            }
        }

        context->OMSetRenderTargets(1, nullRenderTarget, nullptr);
        context->PSSetShaderResources(0, static_cast<UINT>(std::size(nullSrvs)), nullSrvs);

        D3D11_VIEWPORT viewport{};
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        viewport.Width = static_cast<float>(m_deviceResources->width());
        viewport.Height = static_cast<float>(m_deviceResources->height());
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        context->RSSetViewports(1, &viewport);
    }

    void Renderer::createIrradianceEnvironmentResources()
    {
        ScopedTimer timer(L"Create/load irradiance environment");
        if (loadCubeCache(textureCachePath(kIrradianceEnvironmentCacheFile), m_irradianceEnvironmentShaderResourceView.GetAddressOf()))
        {
            Logger::info(L"Loaded irradiance environment DDS cache.");
            return;
        }

        ID3D11Device* device = m_deviceResources->device();

        D3D11_TEXTURE2D_DESC textureDesc{};
        textureDesc.Width = kIrradianceEnvironmentCubeFaceSize;
        textureDesc.Height = kIrradianceEnvironmentCubeFaceSize;
        textureDesc.MipLevels = 1;
        textureDesc.ArraySize = 6;
        textureDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        textureDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

        throwIfFailed(
            device->CreateTexture2D(&textureDesc, nullptr, m_irradianceEnvironmentTexture.GetAddressOf()),
            "CreateTexture2D irradiance environment failed.");

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = textureDesc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.TextureCube.MipLevels = 1;

        throwIfFailed(
            device->CreateShaderResourceView(
                m_irradianceEnvironmentTexture.Get(),
                &srvDesc,
                m_irradianceEnvironmentShaderResourceView.GetAddressOf()),
            "CreateShaderResourceView irradiance environment failed.");

        m_irradianceEnvironmentRenderTargetViews.clear();
        m_irradianceEnvironmentRenderTargetViews.resize(6);

        for (UINT face = 0; face < 6; ++face)
        {
            D3D11_RENDER_TARGET_VIEW_DESC rtvDesc{};
            rtvDesc.Format = textureDesc.Format;
            rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
            rtvDesc.Texture2DArray.MipSlice = 0;
            rtvDesc.Texture2DArray.FirstArraySlice = face;
            rtvDesc.Texture2DArray.ArraySize = 1;

            throwIfFailed(
                device->CreateRenderTargetView(
                    m_irradianceEnvironmentTexture.Get(),
                    &rtvDesc,
                    m_irradianceEnvironmentRenderTargetViews[face].GetAddressOf()),
                "CreateRenderTargetView irradiance environment failed.");
        }

        renderIrradianceEnvironment();
        saveCubeCache(m_irradianceEnvironmentTexture.Get(), textureCachePath(kIrradianceEnvironmentCacheFile));
    }

    void Renderer::renderIrradianceEnvironment()
    {
        ScopedTimer timer(L"GPU irradiance convolution");
        ID3D11DeviceContext* context = m_deviceResources->context();

        ID3D11RenderTargetView* nullRenderTarget[] = { nullptr };
        ID3D11ShaderResourceView* nullSrvs[] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
        context->PSSetShaderResources(0, static_cast<UINT>(std::size(nullSrvs)), nullSrvs);

        D3D11_VIEWPORT viewport{};
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        viewport.Width = static_cast<float>(kIrradianceEnvironmentCubeFaceSize);
        viewport.Height = static_cast<float>(kIrradianceEnvironmentCubeFaceSize);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        context->RSSetViewports(1, &viewport);

        ID3D11ShaderResourceView* shaderResources[] = { m_environmentCubeShaderResourceView.Get() };
        ID3D11SamplerState* samplers[] = { m_linearSampler.Get() };
        ID3D11Buffer* constantBuffers[] = { m_environmentConvertConstantBuffer.Get() };

        context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
        context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
        context->IASetInputLayout(nullptr);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->VSSetShader(m_environmentIrradianceVertexShader.Get(), nullptr, 0);
        context->PSSetShader(m_environmentIrradiancePixelShader.Get(), nullptr, 0);
        context->PSSetShaderResources(0, 1, shaderResources);
        context->PSSetSamplers(0, 1, samplers);
        context->PSSetConstantBuffers(0, 1, constantBuffers);

        for (UINT face = 0; face < 6; ++face)
        {
            ID3D11RenderTargetView* renderTargets[] = { m_irradianceEnvironmentRenderTargetViews[face].Get() };
            const float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
            context->ClearRenderTargetView(renderTargets[0], clearColor);
            context->OMSetRenderTargets(1, renderTargets, nullptr);

            EnvironmentConvertConstants constants{};
            constants.faceIndex = face;
            context->UpdateSubresource(m_environmentConvertConstantBuffer.Get(), 0, nullptr, &constants, 0, 0);
            context->Draw(3, 0);
        }

        context->OMSetRenderTargets(1, nullRenderTarget, nullptr);
        context->PSSetShaderResources(0, static_cast<UINT>(std::size(nullSrvs)), nullSrvs);

        viewport.Width = static_cast<float>(m_deviceResources->width());
        viewport.Height = static_cast<float>(m_deviceResources->height());
        context->RSSetViewports(1, &viewport);
    }

    void Renderer::createResourcePreviewResources()
    {
        auto createPreviewTexture = [this](
            Microsoft::WRL::ComPtr<ID3D11Texture2D>& texture,
            Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& renderTargetView,
            Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& shaderResourceView)
        {
            D3D11_TEXTURE2D_DESC textureDesc{};
            textureDesc.Width = kResourcePreviewSize;
            textureDesc.Height = kResourcePreviewSize;
            textureDesc.MipLevels = 1;
            textureDesc.ArraySize = 1;
            textureDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
            textureDesc.SampleDesc.Count = 1;
            textureDesc.Usage = D3D11_USAGE_DEFAULT;
            textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

            throwIfFailed(
                m_deviceResources->device()->CreateTexture2D(&textureDesc, nullptr, texture.GetAddressOf()),
                "CreateTexture2D resource preview failed.");
            throwIfFailed(
                m_deviceResources->device()->CreateRenderTargetView(texture.Get(), nullptr, renderTargetView.GetAddressOf()),
                "CreateRenderTargetView resource preview failed.");
            throwIfFailed(
                m_deviceResources->device()->CreateShaderResourceView(texture.Get(), nullptr, shaderResourceView.GetAddressOf()),
                "CreateShaderResourceView resource preview failed.");
        };

        createPreviewTexture(
            m_irradiancePreviewTexture,
            m_irradiancePreviewRenderTargetView,
            m_irradiancePreviewShaderResourceView);
        createPreviewTexture(
            m_prefilteredPreviewTexture,
            m_prefilteredPreviewRenderTargetView,
            m_prefilteredPreviewShaderResourceView);

        m_previewResources.brdfLut = m_brdfLutShaderResourceView.Get();
        m_previewResources.irradiance = m_irradiancePreviewShaderResourceView.Get();
        m_previewResources.prefiltered = m_prefilteredPreviewShaderResourceView.Get();
    }

    void Renderer::renderResourcePreviews(const DebugSettings& settings)
    {
        const unsigned int faceIndex = static_cast<unsigned int>((std::max)(0, (std::min)(settings.previewCubeFace, 5)));
        renderCubePreview(m_irradianceEnvironmentShaderResourceView.Get(), m_irradiancePreviewRenderTargetView.Get(), faceIndex, 0.0f);
        renderCubePreview(
            m_prefilteredEnvironmentShaderResourceView.Get(),
            m_prefilteredPreviewRenderTargetView.Get(),
            faceIndex,
            settings.previewPrefilterMip);
        m_deviceResources->setBackBufferRenderTarget();
    }

    void Renderer::renderCubePreview(
        ID3D11ShaderResourceView* cubeShaderResourceView,
        ID3D11RenderTargetView* previewRenderTargetView,
        unsigned int faceIndex,
        float mipLevel)
    {
        ID3D11DeviceContext* context = m_deviceResources->context();
        ID3D11RenderTargetView* renderTargets[] = { previewRenderTargetView };
        ID3D11ShaderResourceView* shaderResources[] = { cubeShaderResourceView };
        ID3D11SamplerState* samplers[] = { m_linearSampler.Get() };
        ID3D11Buffer* constantBuffers[] = { m_cubePreviewConstantBuffer.Get() };

        CubePreviewConstants constants{};
        constants.faceIndex = faceIndex;
        constants.mipLevel = mipLevel;
        context->UpdateSubresource(m_cubePreviewConstantBuffer.Get(), 0, nullptr, &constants, 0, 0);

        D3D11_VIEWPORT viewport{};
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        viewport.Width = static_cast<float>(kResourcePreviewSize);
        viewport.Height = static_cast<float>(kResourcePreviewSize);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        context->RSSetViewports(1, &viewport);

        context->OMSetRenderTargets(1, renderTargets, nullptr);
        context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
        context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
        context->IASetInputLayout(nullptr);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->VSSetShader(m_cubePreviewVertexShader.Get(), nullptr, 0);
        context->PSSetShader(m_cubePreviewPixelShader.Get(), nullptr, 0);
        context->PSSetConstantBuffers(0, 1, constantBuffers);
        context->PSSetShaderResources(0, 1, shaderResources);
        context->PSSetSamplers(0, 1, samplers);
        context->Draw(3, 0);

        ID3D11RenderTargetView* nullRenderTarget[] = { nullptr };
        ID3D11ShaderResourceView* nullSrvs[] = { nullptr };
        context->OMSetRenderTargets(1, nullRenderTarget, nullptr);
        context->PSSetShaderResources(0, 1, nullSrvs);

        viewport.Width = static_cast<float>(m_deviceResources->width());
        viewport.Height = static_cast<float>(m_deviceResources->height());
        context->RSSetViewports(1, &viewport);
    }

    bool Renderer::loadCubeCache(const std::filesystem::path& path, ID3D11ShaderResourceView** shaderResourceView)
    {
        if (!std::filesystem::exists(path))
        {
            return false;
        }

        ScopedTimer timer(L"Load DDS cache " + path.filename().wstring());
        DirectX::ScratchImage image;
        DirectX::TexMetadata metadata{};
        throwIfFailed(
            DirectX::LoadFromDDSFile(path.c_str(), DirectX::DDS_FLAGS_NONE, &metadata, image),
            "LoadFromDDSFile cache failed.");
        throwIfFailed(
            DirectX::CreateShaderResourceView(
                m_deviceResources->device(),
                image.GetImages(),
                image.GetImageCount(),
                metadata,
                shaderResourceView),
            "CreateShaderResourceView cache failed.");
        return true;
    }

    void Renderer::saveCubeCache(ID3D11Resource* resource, const std::filesystem::path& path)
    {
        ScopedTimer timer(L"Save DDS cache " + path.filename().wstring());
        std::filesystem::create_directories(path.parent_path());

        DirectX::ScratchImage image;
        throwIfFailed(
            DirectX::CaptureTexture(m_deviceResources->device(), m_deviceResources->context(), resource, image),
            "CaptureTexture cache failed.");
        throwIfFailed(
            DirectX::SaveToDDSFile(
                image.GetImages(),
                image.GetImageCount(),
                image.GetMetadata(),
                DirectX::DDS_FLAGS_NONE,
                path.c_str()),
            "SaveToDDSFile cache failed.");
    }

    void Renderer::updateObjectConstants(float totalSeconds, const Camera& camera, const DebugSettings& settings)
    {
        updateLightConstants(settings);

        const XMMATRIX world = settings.rotateObject
            ? XMMatrixRotationY(totalSeconds) * XMMatrixRotationX(totalSeconds * 0.35f)
            : XMMatrixIdentity();
        const XMMATRIX view = XMLoadFloat4x4(&camera.view());
        const XMMATRIX projection = XMLoadFloat4x4(&camera.projection());

        ObjectConstants constants{};
        XMStoreFloat4x4(&constants.worldViewProjection, XMMatrixTranspose(world * view * projection));
        XMStoreFloat4x4(&constants.world, XMMatrixTranspose(world));
        constants.cameraPosition = camera.position();
        m_deviceResources->context()->UpdateSubresource(m_objectConstantBuffer.Get(), 0, nullptr, &constants, 0, 0);
    }

    void Renderer::updateMaterialConstants()
    {
        m_deviceResources->context()->UpdateSubresource(m_materialConstantBuffer.Get(), 0, nullptr, &m_material.constants, 0, 0);
    }

    void Renderer::updateCompositeConstants(const Camera& camera, const DebugSettings& settings)
    {
        const XMMATRIX view = XMLoadFloat4x4(&camera.view());
        const XMMATRIX projection = XMLoadFloat4x4(&camera.projection());
        const XMMATRIX inverseViewProjection = XMMatrixInverse(nullptr, view * projection);

        CompositeConstants constants{};
        XMStoreFloat4x4(&constants.inverseViewProjection, XMMatrixTranspose(inverseViewProjection));
        constants.cameraPosition = camera.position();
        constants.exposure = settings.exposure;
        constants.backgroundIntensity = settings.backgroundIntensity;
        constants.showEnvironmentBackground = settings.showEnvironmentBackground ? 1.0f : 0.0f;
        m_deviceResources->context()->UpdateSubresource(m_compositeConstantBuffer.Get(), 0, nullptr, &constants, 0, 0);
    }

    void Renderer::updateLightConstants(const DebugSettings& settings)
    {
        const XMVECTOR lightDirection = XMVector3Normalize(XMLoadFloat3(&settings.lightDirection));

        LightConstants constants{};
        XMStoreFloat3(&constants.direction, lightDirection);
        constants.intensity = settings.lightIntensity;
        constants.color = settings.lightColor;
        constants.diffuseIblStrength = settings.diffuseIblStrength;
        constants.skyColor = settings.skyColor;
        constants.iblEnabled = settings.iblEnabled ? 1.0f : 0.0f;
        constants.groundColor = settings.groundColor;
        constants.environmentMipCount = kPrefilteredEnvironmentCubeMipCount;
        constants.specularIblStrength = settings.specularIblStrength;
        m_deviceResources->context()->UpdateSubresource(m_lightConstantBuffer.Get(), 0, nullptr, &constants, 0, 0);
    }

    std::filesystem::path Renderer::shaderPath(const wchar_t* fileName) const
    {
        const std::filesystem::path fromWorkingDirectory = std::filesystem::path(L"assets") / L"shaders" / fileName;
        if (std::filesystem::exists(fromWorkingDirectory))
        {
            return fromWorkingDirectory;
        }

        const std::filesystem::path fromOutputDirectory = std::filesystem::path(L"..") / L".." / L"assets" / L"shaders" / fileName;
        if (std::filesystem::exists(fromOutputDirectory))
        {
            return fromOutputDirectory;
        }

        return std::filesystem::path(L"..") / L"assets" / L"shaders" / fileName;
    }

    std::filesystem::path Renderer::texturePath(const wchar_t* fileName) const
    {
        const std::filesystem::path fromWorkingDirectory = std::filesystem::path(L"assets") / L"textures" / fileName;
        if (std::filesystem::exists(fromWorkingDirectory))
        {
            return fromWorkingDirectory;
        }

        const std::filesystem::path fromOutputDirectory = std::filesystem::path(L"..") / L".." / L"assets" / L"textures" / fileName;
        if (std::filesystem::exists(fromOutputDirectory))
        {
            return fromOutputDirectory;
        }

        return std::filesystem::path(L"..") / L"assets" / L"textures" / fileName;
    }

    std::filesystem::path Renderer::textureCachePath(const wchar_t* fileName) const
    {
        const std::filesystem::path directory = std::filesystem::path(L"assets") / L"textures";
        if (std::filesystem::exists(directory))
        {
            return directory / fileName;
        }

        return std::filesystem::path(L"..") / L"assets" / L"textures" / fileName;
    }
}
