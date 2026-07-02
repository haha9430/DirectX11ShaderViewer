#pragma once

#include "core/MathTypes.h"
#include "renderer/Texture.h"

#include <filesystem>

namespace dxsv
{
    struct PbrTexturePaths
    {
        std::filesystem::path albedo;
        std::filesystem::path normal;
        std::filesystem::path metallic;
        std::filesystem::path roughness;
        std::filesystem::path ao;
    };

    struct Material
    {
        struct Constants
        {
            Vector4 baseColorFactor{ 1.0f, 1.0f, 1.0f, 1.0f };
            float metallicFactor = 1.0f;
            float roughnessFactor = 1.0f;
            float aoStrength = 1.0f;
            float normalMapStrength = 1.0f;
            int debugViewMode = 0;
            Vector3 padding{ 0.0f, 0.0f, 0.0f };
        };

        void loadPbrTextures(ID3D11Device* device, const PbrTexturePaths& paths);

        Texture albedo;
        Texture normal;
        Texture metallic;
        Texture roughness;
        Texture ao;
        Constants constants;
    };
}
