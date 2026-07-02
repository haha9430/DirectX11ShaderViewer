# Render Target System

## Purpose

RenderTarget은 장면을 바로 BackBuffer에 그리지 않고 별도의 Texture에 렌더링하기 위한 구조이다.

## Use Cases

- Scene rendering
- Shadow map generation
- Post-processing
- Bloom extraction
- Final composition

## Render Flow

1. Render scene to SceneRenderTarget
2. Render bright areas to BloomExtractTarget
3. Apply Gaussian blur
4. Composite final image to BackBuffer