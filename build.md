# Build Guide

## Requirements

- Windows 10/11
- Visual Studio 2022
- MSVC v143
- Windows SDK
- C++17

## Required Libraries

- d3d11.lib
- dxgi.lib
- d3dcompiler.lib
- windowscodecs.lib

## Build Steps

1. Open `DirectX11ShaderViewer.sln`
2. Set platform to `x64`
3. Set configuration to `Debug` or `Release`
4. Build solution
5. Run executable

## Notes

- Do not use x86 configuration.
- Shader files should be copied to the output directory.
- Texture assets should be placed under `assets/textures`.