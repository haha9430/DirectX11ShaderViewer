# AGENTS.md

## Project Goal

This project is a DirectX 11 based 3D shader rendering viewer.
The goal is to demonstrate real-time rendering fundamentals such as texture mapping, lighting, shader switching, and debug UI.

## Main Constraints

- Use C++17.
- Use DirectX 11.
- Use HLSL for shader code.
- Keep the project small and portfolio-oriented.
- Avoid over-engineering.
- Do not implement a full game engine.
- Do not add complex model loading unless core rendering features are complete.

## Development Priority

1. DirectX 11 initialization
2. Textured cube rendering
3. Camera control
4. Blinn-Phong lighting
5. Shader switching
6. ImGui debug UI
7. Rim lighting / Toon shading / Normal mapping
8. Post-processing if time remains

## Code Style

- Prefer clear class separation.
- Keep rendering-related code inside `src/renderer`.
- Keep scene and object data inside `src/scene`.
- Keep platform/window code inside `src/app`.
- Keep utility code inside `src/core`.

## Do Not

- Do not introduce ECS.
- Do not introduce multithreaded rendering.
- Do not add Vulkan, OpenGL, or DirectX 12.
- Do not use large external engines.
- Do not spend time on complex asset pipelines.