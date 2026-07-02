# Asset Pipeline

## Shader Assets

Shader files are stored in `assets/shaders`.

Important PBR/IBL shaders:

- `PBR.hlsl`
- `FinalComposite.hlsl`
- `BrdfLut.hlsl`
- `EnvironmentConvert.hlsl`
- `EnvironmentPrefilter.hlsl`
- `EnvironmentIrradiance.hlsl`
- `CubePreview.hlsl`

## Texture Assets

Texture files are stored in `assets/textures`.

Material textures are grouped by folder:

- `silver-bl`
- `light-gold-bl`
- `bronze-bl`

The renderer expects albedo, normal, metallic, roughness, and optionally AO textures. Missing AO maps use a white fallback texture.

## HDR Environment Flow

The source environment began as an EXR file. It is converted to an 8K Radiance HDR file and loaded through DirectXTex.

```text
028_hdrmaps_com_free_10K.exr
-> environment_8k.hdr
-> DirectXTex LoadFromHDRFile
-> GPU equirectangular to cubemap
-> GPU prefilter
-> GPU irradiance convolution
-> DDS cache
```

## DirectXTex Usage

DirectXTex is used for HDR/DDS handling because WIC is not a good fit for floating point HDR environment data.

- `LoadFromHDRFile`: load HDR equirectangular environment
- `LoadFromDDSFile`: load generated caches
- `CaptureTexture`: read back generated cubemaps
- `SaveToDDSFile`: persist generated cubemap caches

## Cache Files

Generated cache files live in `assets/textures`.

- `environment_cube_cache.dds`
- `environment_prefiltered_cache.dds`
- `environment_irradiance_cache.dds`

When these files exist, the app skips expensive environment generation and loads the DDS files directly.
