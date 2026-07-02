# Architecture

## Overview

이 프로젝트는 DirectX 11 기반 렌더링 뷰어이며, 크게 App, Renderer, Scene, Debug, Core 계층으로 구성된다.

## Layers

### App Layer

Win32 창 생성, 메시 루프, D3DApp 실행을 담당한다.

### Renderer Layer

DirectX 11 디바이스, 스왑체인, 셰이더, 텍스처, 메시, 렌더 타겟을 관리한다.

### Scene Layer

카메라, 라이트, 렌더링 대상 오브젝트를 관리한다.

### Debug Layer

ImGui 기반 디버그 UI와 렌더링 상태 시각화를 담당한다.

### Core Layer

Timer, Input, Logger, Math utility를 관리한다.

## Frame Flow

1. Process window messages
2. Update timer
3. Update input
4. Update camera
5. Update scene objects
6. Bind render targets
7. Update constant buffers
8. Draw scene
9. Draw ImGui
10. Present swap chain