# Material System

## Purpose

Material은 PBR 셰이더가 사용할 표면 파라미터와 텍스처를 묶어서 관리한다.

## Texture Materials

현재 텍스처 기반 재질은 `assets/textures` 아래의 재질 폴더에서 로드한다.

- `silver-bl`
- `light-gold-bl`
- `bronze-bl`

각 재질은 albedo, normal, metallic, roughness, ao 텍스처를 사용할 수 있다. AO 텍스처가 없는 경우에는 흰색 1x1 fallback 텍스처를 사용한다.

## Shader Inputs

`MaterialConstants`는 다음 값을 전달한다.

- Base color factor
- Metallic factor
- Roughness factor
- AO strength
- Normal map strength
- PBR debug view mode

Renderer는 선택된 texture set을 로드하고, factor 값들을 텍스처 샘플에 곱해서 `PBR.hlsl`에서 최종 shading에 사용한다.
