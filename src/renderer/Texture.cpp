#include "renderer/Texture.h"

#include "core/Common.h"

#include <utility>
#include <vector>
#include <wincodec.h>

namespace dxsv
{
    namespace
    {
        struct ImageData
        {
            UINT width = 0;
            UINT height = 0;
            std::vector<unsigned char> pixels;
        };

        ImageData loadRgbaImage(const std::filesystem::path& path)
        {
            Microsoft::WRL::ComPtr<IWICImagingFactory> factory;
            throwIfFailed(
                CoCreateInstance(
                    CLSID_WICImagingFactory,
                    nullptr,
                    CLSCTX_INPROC_SERVER,
                    IID_PPV_ARGS(factory.GetAddressOf())),
                "CoCreateInstance IWICImagingFactory failed.");

            Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
            throwIfFailed(
                factory->CreateDecoderFromFilename(
                    path.c_str(),
                    nullptr,
                    GENERIC_READ,
                    WICDecodeMetadataCacheOnLoad,
                    decoder.GetAddressOf()),
                "CreateDecoderFromFilename failed.");

            Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
            throwIfFailed(decoder->GetFrame(0, frame.GetAddressOf()), "IWICBitmapDecoder::GetFrame failed.");

            ImageData image{};
            throwIfFailed(frame->GetSize(&image.width, &image.height), "IWICBitmapFrameDecode::GetSize failed.");

            Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
            throwIfFailed(factory->CreateFormatConverter(converter.GetAddressOf()), "CreateFormatConverter failed.");
            throwIfFailed(
                converter->Initialize(
                    frame.Get(),
                    GUID_WICPixelFormat32bppRGBA,
                    WICBitmapDitherTypeNone,
                    nullptr,
                    0.0,
                    WICBitmapPaletteTypeCustom),
                "IWICFormatConverter::Initialize failed.");

            image.pixels.resize(static_cast<size_t>(image.width) * static_cast<size_t>(image.height) * 4);
            const UINT stride = image.width * 4;
            throwIfFailed(
                converter->CopyPixels(nullptr, stride, static_cast<UINT>(image.pixels.size()), image.pixels.data()),
                "IWICFormatConverter::CopyPixels failed.");
            return image;
        }
    }

    Texture::Texture(std::filesystem::path path)
        : m_path(std::move(path))
    {
    }

    void Texture::loadFromFile(ID3D11Device* device, const std::filesystem::path& path)
    {
        m_path = path;
        m_shaderResourceView.Reset();

        const ImageData image = loadRgbaImage(path);
        const UINT stride = image.width * 4;

        D3D11_TEXTURE2D_DESC textureDesc{};
        textureDesc.Width = image.width;
        textureDesc.Height = image.height;
        textureDesc.MipLevels = 0;
        textureDesc.ArraySize = 1;
        textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        textureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
        throwIfFailed(
            device->CreateTexture2D(&textureDesc, nullptr, texture.GetAddressOf()),
            "CreateTexture2D texture failed.");

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = textureDesc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = -1;

        throwIfFailed(
            device->CreateShaderResourceView(texture.Get(), &srvDesc, m_shaderResourceView.GetAddressOf()),
            "CreateShaderResourceView failed.");

        Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
        device->GetImmediateContext(context.GetAddressOf());
        context->UpdateSubresource(texture.Get(), 0, nullptr, image.pixels.data(), stride, 0);
        context->GenerateMips(m_shaderResourceView.Get());
    }

    void Texture::createSolidColor(ID3D11Device* device, unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha)
    {
        m_path.clear();
        m_shaderResourceView.Reset();

        const unsigned char pixels[] = { red, green, blue, alpha };

        D3D11_TEXTURE2D_DESC textureDesc{};
        textureDesc.Width = 1;
        textureDesc.Height = 1;
        textureDesc.MipLevels = 1;
        textureDesc.ArraySize = 1;
        textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA initialData{};
        initialData.pSysMem = pixels;
        initialData.SysMemPitch = 4;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
        throwIfFailed(
            device->CreateTexture2D(&textureDesc, &initialData, texture.GetAddressOf()),
            "CreateTexture2D solid texture failed.");

        throwIfFailed(
            device->CreateShaderResourceView(texture.Get(), nullptr, m_shaderResourceView.GetAddressOf()),
            "CreateShaderResourceView solid texture failed.");
    }
}
