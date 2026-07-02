# Rendering Pipeline

## Initialization

초기화 단계에서는 DirectX 11 device, swap chain, back buffer, depth buffer를 만든 뒤 PBR 렌더링에 필요한 GPU 리소스를 준비한다.

1. Compile HLSL shaders
2. Create sphere mesh
3. Create HDR scene render target
4. Generate BRDF LUT on GPU
5. Load material textures
6. Load or generate environment cubemap
7. Load or generate prefiltered cubemap
8. Load or generate irradiance cubemap
9. Create resource preview render targets

## Frame Rendering Order

1. Apply ImGui debug settings
2. Update object, material, light constant buffers
3. Render PBR sphere into HDR render target
4. Composite HDR scene and environment background to back buffer
5. Render resource preview thumbnails
6. Render ImGui
7. Present swap chain

## PBR/IBL Flow

The renderer uses split-sum image based lighting.

- Direct lighting: GGX NDF, Smith geometry, Schlick Fresnel
- Diffuse IBL: irradiance cubemap
- Specular IBL: prefiltered environment cubemap
- BRDF integration: GPU generated 2D BRDF LUT

Diffuse IBL strength and specular IBL strength are exposed separately in the debug UI.

## Render Targets

- Back buffer: final LDR output
- HDR scene target: PBR sphere color before tone mapping
- BRDF LUT: 2D `R16G16_FLOAT` integration texture
- Resource previews: small 2D render targets used by ImGui thumbnails
