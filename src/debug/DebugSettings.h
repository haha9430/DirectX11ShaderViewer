#pragma once

#include "core/MathTypes.h"

namespace dxsv
{
    enum class ShaderMode
    {
        BasicTexture,
        Lambert,
        BlinnPhong,
        RimLight,
        ToonShading,
        NormalMapping,
        Pbr
    };

    enum class PbrDebugView
    {
        Final = 0,
        Albedo,
        Normal,
        Metallic,
        Roughness,
        Ao,
        DirectLighting,
        DiffuseIbl,
        SpecularIbl,
        Irradiance,
        PrefilteredSpecular,
        BrdfLut
    };

    enum class TextureMaterialSet
    {
        Silver,
        LightGold,
        Bronze
    };

    struct DebugSettings
    {
        ShaderMode shaderMode = ShaderMode::Pbr;
        bool wireframe = false;
        bool normalMapping = true;
        bool rotateObject = true;
        PbrDebugView pbrDebugView = PbrDebugView::Final;
        TextureMaterialSet textureMaterialSet = TextureMaterialSet::Silver;

        Vector3 baseColor{ 1.0f, 1.0f, 1.0f };
        float metallicFactor = 1.0f;
        float roughnessFactor = 1.0f;
        float aoStrength = 1.0f;
        float normalMapStrength = 1.0f;
        float exposure = 1.15f;

        Vector3 lightDirection{ -0.35f, 1.0f, -0.25f };
        Vector3 lightColor{ 1.0f, 1.0f, 1.0f };
        float lightIntensity = 4.0f;

        bool iblEnabled = true;
        bool showEnvironmentBackground = true;
        float diffuseIblStrength = 1.0f;
        float specularIblStrength = 1.0f;
        float backgroundIntensity = 1.0f;
        Vector3 skyColor{ 0.38f, 0.48f, 0.62f };
        Vector3 groundColor{ 0.035f, 0.032f, 0.028f };
        int previewCubeFace = 0;
        float previewPrefilterMip = 0.0f;
    };
}
