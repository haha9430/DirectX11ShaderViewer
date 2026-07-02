# DirectX 11 Shader Viewer

A real-time PBR shader viewer built with DirectX 11 and HLSL.  
The project focuses on HDR environment lighting, GPU precompute passes, material texture set switching, and ImGui-based rendering debug tools.

## Demo

### Silver

![Silver material preview](image/image01.png)

### Light Gold

![Light gold material preview](image/image02.png)

### Bronze

![Bronze material preview](image/image03.png)

## Features

- DirectX 11 device, swap chain, render target, depth stencil setup
- WASD / mouse camera control
- PBR sphere rendering with albedo, normal, metallic, roughness, AO textures
- Silver, Light Gold, Bronze material texture set switching
- HDR environment background and image based lighting
- GPU equirectangular-to-cubemap conversion
- GPU GGX prefiltered environment cubemap
- GPU irradiance cubemap generation
- GPU BRDF LUT generation
- DDS cache loading/saving with DirectXTex
- ImGui PBR debug view
- BRDF LUT, irradiance, prefiltered environment resource preview
- Window resize handling

## Tech Stack

- C++17
- DirectX 11
- HLSL Shader Model 5.0
- Visual Studio 2022
- Dear ImGui
- DirectXTex
- Windows Imaging Component

## Build

### Requirements

- Windows 10/11
- Visual Studio 2022
- MSVC v143
- Windows SDK
- x64 platform

### Steps

1. Open `DirectX11ShaderViewer.sln`.
2. Select `x64`.
3. Select `Debug` or `Release`.
4. Build the solution.
5. Run `DirectX11ShaderViewer.exe` from the repository root or Visual Studio.

The app expects runtime assets under `assets/shaders` and `assets/textures`.

## Controls

- `WASD`: move camera
- `Space / Ctrl`: move camera up/down
- `Right Mouse Drag`: rotate camera
- `F1`: toggle normal mapping
- `F2`: toggle object rotation
- `Z / X`: decrease/increase metallic factor
- `C / V`: decrease/increase roughness factor
- `B / N`: decrease/increase normal strength
- `G / H`: decrease/increase AO strength
- `- / +`: decrease/increase exposure

## Debug UI

The ImGui panel exposes:

- PBR debug views: final, albedo, normal, metallic, roughness, AO, direct lighting, diffuse IBL, specular IBL, irradiance, prefiltered specular, BRDF LUT
- Material texture set selection: Silver, Light Gold, Bronze
- Direct light, diffuse IBL, specular IBL, background intensity, exposure controls
- Resource preview for BRDF LUT, irradiance cubemap, and prefiltered environment cubemap

## Asset Notes

Large generated environment files are intentionally excluded from Git:

- `assets/textures/*cache.dds`
- `assets/textures/*.exr`
- `assets/textures/environment_8k.hdr`
- generated environment preview PNGs

The environment cubemap, prefiltered cubemap, and irradiance cubemap are generated or loaded from DDS cache at runtime.

## Project Structure

```text
assets/shaders      HLSL shader files
assets/textures     Material textures and local environment assets
docs                System notes and portfolio writeups
external            Third-party libraries
src/app             Win32 and app loop
src/core            Input, timer, logging, utilities
src/debug           ImGui and debug settings
src/renderer        DirectX resources, renderer, materials, textures
src/scene           Camera, light, scene objects
src/systems         Camera/light/post-process controller shells
image               README screenshots
```

## What I Learned

- How to organize a small DirectX 11 renderer without turning it into a full engine
- How PBR combines direct lighting, diffuse IBL, specular IBL, and BRDF LUT terms
- How HDR environment maps are converted into runtime cubemap resources
- How GPU precompute passes can replace slow CPU-side environment processing
- How ImGui debug views make shader validation much easier

## Future Improvements

- Shadow mapping
- Bloom/post-processing polish
- Environment selection and cache rebuild UI
- Optional model loading after core rendering is stable
