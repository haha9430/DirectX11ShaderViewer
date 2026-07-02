#include "renderer/Material.h"

#include <filesystem>

namespace dxsv
{
    void Material::loadPbrTextures(ID3D11Device* device, const PbrTexturePaths& paths)
    {
        albedo.loadFromFile(device, paths.albedo);
        normal.loadFromFile(device, paths.normal);
        metallic.loadFromFile(device, paths.metallic);
        roughness.loadFromFile(device, paths.roughness);
        if (std::filesystem::exists(paths.ao))
        {
            ao.loadFromFile(device, paths.ao);
        }
        else
        {
            ao.createSolidColor(device, 255, 255, 255, 255);
        }
    }
}
