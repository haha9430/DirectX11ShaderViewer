# Rendering Pipeline

## Frame Rendering Order

1. Clear render target
2. Clear depth stencil
3. Update camera constant buffer
4. Update light constant buffer
5. Bind vertex buffer
6. Bind index buffer
7. Bind input layout
8. Bind vertex shader
9. Bind pixel shader
10. Bind texture and sampler
11. Draw indexed mesh
12. Render ImGui
13. Present swap chain

## DirectX 11 Resources

- Device
- DeviceContext
- SwapChain
- RenderTargetView
- DepthStencilView
- RasterizerState
- BlendState
- SamplerState