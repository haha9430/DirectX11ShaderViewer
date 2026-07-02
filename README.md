# DirectX 11 Shader Viewer

DirectX 11과 HLSL로 구현한 실시간 PBR 셰이더 뷰어입니다.  
HDR 환경맵 기반 IBL, GPU precompute pass, PBR 텍스처 재질 전환, ImGui 디버그 UI를 중심으로 구성했습니다.

## 데모

### Silver

![Silver material preview](image/image01.png)

### Light Gold

![Light gold material preview](image/image02.png)

### Bronze

![Bronze material preview](image/image03.png)

## 주요 기능

- DirectX 11 device, swap chain, render target, depth stencil 초기화
- WASD / 마우스 기반 자유 카메라
- albedo, normal, metallic, roughness, AO 텍스처를 사용하는 PBR 구 렌더링
- Silver, Light Gold, Bronze 재질 텍스처셋 전환
- HDR environment background와 image based lighting
- GPU equirectangular-to-cubemap 변환
- GPU GGX prefiltered environment cubemap 생성
- GPU irradiance cubemap 생성
- GPU BRDF LUT 생성
- DirectXTex 기반 DDS cache 로드/저장
- ImGui 기반 PBR debug view
- BRDF LUT, irradiance cubemap, prefiltered environment resource preview
- 창 크기 변경 대응

## 기술 스택

- C++17
- DirectX 11
- HLSL Shader Model 5.0
- Visual Studio 2022
- Dear ImGui
- DirectXTex
- Windows Imaging Component

## 빌드 방법

### 요구 사항

- Windows 10/11
- Visual Studio 2022
- MSVC v143
- Windows SDK
- x64 플랫폼

### 빌드 순서

1. `DirectX11ShaderViewer.sln`을 Visual Studio에서 엽니다.
2. 플랫폼을 `x64`로 설정합니다.
3. 구성을 `Debug` 또는 `Release`로 설정합니다.
4. 솔루션을 빌드합니다.
5. Visual Studio에서 실행하거나 저장소 루트 기준으로 실행 파일을 실행합니다.

런타임에는 `assets/shaders`, `assets/textures` 폴더의 리소스가 필요합니다.

## 조작법

- `WASD`: 카메라 이동
- `Space / Ctrl`: 카메라 위/아래 이동
- `마우스 우클릭 드래그`: 카메라 회전
- `F1`: normal mapping 토글
- `F2`: 오브젝트 회전 토글
- `Z / X`: metallic factor 감소/증가
- `C / V`: roughness factor 감소/증가
- `B / N`: normal strength 감소/증가
- `G / H`: AO strength 감소/증가
- `- / +`: exposure 감소/증가

## 디버그 UI

ImGui 패널에서 다음 항목을 실시간으로 조절하거나 확인할 수 있습니다.

- PBR debug view: final, albedo, normal, metallic, roughness, AO, direct lighting, diffuse IBL, specular IBL, irradiance, prefiltered specular, BRDF LUT
- 재질 텍스처셋 선택: Silver, Light Gold, Bronze
- direct light, diffuse IBL, specular IBL, background intensity, exposure 조절
- BRDF LUT, irradiance cubemap, prefiltered environment cubemap preview

## 에셋 관련 메모

용량이 큰 environment 원본 및 생성 캐시는 Git에 포함하지 않습니다.

- `assets/textures/*cache.dds`
- `assets/textures/*.exr`
- `assets/textures/environment_8k.hdr`
- 생성된 environment preview PNG 파일

environment cubemap, prefiltered cubemap, irradiance cubemap은 런타임에서 생성하거나 DDS cache가 존재할 경우 cache를 로드합니다.

## 프로젝트 구조

```text
assets/shaders      HLSL 셰이더 파일
assets/textures     재질 텍스처 및 로컬 environment 리소스
docs                시스템 문서 및 포트폴리오 정리
external            외부 라이브러리
src/app             Win32 창 및 애플리케이션 루프
src/core            입력, 타이머, 로깅, 유틸리티
src/debug           ImGui 및 debug settings
src/renderer        DirectX 리소스, 렌더러, 재질, 텍스처
src/scene           카메라, 조명, 씬 오브젝트
src/systems         카메라/조명/후처리 컨트롤러
image               README 스크린샷
```

## 배운 점

- 작은 DirectX 11 렌더러를 과도한 엔진 구조 없이 구성하는 방법
- PBR에서 direct lighting, diffuse IBL, specular IBL, BRDF LUT가 결합되는 방식
- HDR environment map을 runtime cubemap 리소스로 변환하는 흐름
- CPU 처리 대신 GPU precompute pass를 사용해 environment 리소스를 생성하는 방법
- ImGui debug view를 활용해 셰이더 중간 결과를 검증하는 방법

## 향후 개선 사항

- Shadow mapping
- Bloom 및 post-processing polish
- Environment 선택 및 cache rebuild UI
- 핵심 렌더링 안정화 이후 optional model loading
