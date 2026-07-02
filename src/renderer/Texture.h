#pragma once

#include <d3d11.h>
#include <filesystem>
#include <wrl/client.h>

namespace dxsv
{
    class Texture
    {
    public:
        Texture() = default;
        explicit Texture(std::filesystem::path path);

        void loadFromFile(ID3D11Device* device, const std::filesystem::path& path);
        void createSolidColor(ID3D11Device* device, unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha);

        ID3D11ShaderResourceView* shaderResourceView() const { return m_shaderResourceView.Get(); }
        const std::filesystem::path& path() const { return m_path; }

    private:
        std::filesystem::path m_path;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_shaderResourceView;
    };
}
