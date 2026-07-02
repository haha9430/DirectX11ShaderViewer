# Coding Convention

## Language

- Use C++17.
- Prefer RAII.
- Use `Microsoft::WRL::ComPtr` for DirectX COM objects.

## Naming

- Classes: PascalCase
- Functions: camelCase
- Member variables: m_camelCase
- Constants: kPascalCase

## Header Rules

- Use `#pragma once`.
- Keep headers minimal.
- Avoid unnecessary includes in header files.

## DirectX Rules

- Store COM objects with `ComPtr`.
- Check HRESULT with helper functions.
- Separate resource creation from rendering logic.