# ImGui Debug UI

## Purpose

셰이더, 재질, 조명, IBL 파라미터를 실시간으로 조절하기 위한 디버그 UI이다.

## PBR Debug View

Debug View combo는 PBR 최종 결과와 중간 계산 결과를 화면에 출력한다.

- Final PBR
- Albedo
- Normal
- Metallic
- Roughness
- AO
- Direct Lighting
- Diffuse IBL
- Specular IBL
- Irradiance
- Prefiltered Specular
- BRDF LUT

## Material Controls

- Texture Set: Silver / Light Gold / Bronze
- Tint
- Metallic
- Roughness
- AO Strength
- Normal Mapping
- Normal Strength

Texture Set은 실제 PBR 텍스처를 교체한다. Tint와 factor 값들은 선택된 텍스처 샘플에 곱해져 재질을 미세 조정하는 용도로 사용된다.

## Lighting and Environment

- Light Direction
- Light Color
- Direct Light
- IBL on/off
- Diffuse IBL
- Specular IBL
- Background Intensity
- Exposure

Diffuse IBL과 Specular IBL은 분리되어 있어 irradiance와 reflection contribution을 따로 검증할 수 있다.

## Resource Preview

Resource Preview 창은 debug view와 별개로 작은 썸네일을 표시한다.

- BRDF LUT
- Irradiance cubemap face
- Prefiltered cubemap face/mip

Cube Face와 Prefilter Mip 슬라이더로 preview 대상 face와 mip level을 선택할 수 있다.
