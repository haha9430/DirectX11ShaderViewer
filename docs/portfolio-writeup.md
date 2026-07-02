# Portfolio Writeup

## Project Title

DirectX 11 PBR Shader Viewer

## Description

DirectX 11과 HLSL을 사용하여 PBR 재질, HDR environment lighting, image based lighting, GPU precompute pass, ImGui debug tooling을 구현한 실시간 렌더링 뷰어입니다.

## Key Contributions

- DirectX 11 device, swap chain, render target, depth stencil 구성
- GGX/Smith/Schlick 기반 PBR shader 구현
- Silver, Light Gold, Bronze PBR material texture set 지원
- HDR environment를 cubemap으로 변환하는 GPU pass 구현
- GGX prefiltered cubemap과 irradiance cubemap 생성
- BRDF LUT를 GPU fullscreen pass로 생성
- DirectXTex 기반 HDR/DDS 로드 및 cubemap cache 저장
- ImGui 기반 PBR debug view와 resource preview 구현

## Portfolio Demonstration Checklist

- Silver, Light Gold, Bronze texture material 비교
- Metallic/Roughness 조절에 따른 반사 변화 확인
- Diffuse IBL과 Specular IBL을 분리해서 contribution 확인
- BRDF LUT, irradiance, prefiltered cubemap preview 확인
- HDR background intensity와 exposure 조절 확인

## Technical Keywords

C++, DirectX 11, HLSL, PBR, GGX, Image Based Lighting, HDR, DirectXTex, DDS Cache, ImGui, Real-time Rendering
