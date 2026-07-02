# Troubleshooting

## Shader compile failed

### Cause

Shader path was incorrect or entry point name was mismatched.

### Solution

Check shader file path, entry point name, and shader model version.

## Texture not displayed

### Cause

SamplerState or ShaderResourceView was not bound.

### Solution

Check `PSSetShaderResources` and `PSSetSamplers`.

## Object appears black

### Cause

Normal vector or light direction was incorrect.

### Solution

Normalize normal and light direction in shader.