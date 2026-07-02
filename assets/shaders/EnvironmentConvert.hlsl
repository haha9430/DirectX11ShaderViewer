static const float PI = 3.14159265f;

Texture2D EquirectangularTexture : register(t0);
SamplerState LinearSampler : register(s0);

cbuffer ConvertConstants : register(b0)
{
    uint FaceIndex;
    float3 Padding;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float2 Texcoord : TEXCOORD0;
};

VSOutput VSMain(uint vertexId : SV_VertexID)
{
    const float2 positions[3] = {
        float2(-1.0f, -1.0f),
        float2(-1.0f,  3.0f),
        float2( 3.0f, -1.0f),
    };

    const float2 texcoords[3] = {
        float2(0.0f, 1.0f),
        float2(0.0f, -1.0f),
        float2(2.0f, 1.0f),
    };

    VSOutput output;
    output.Position = float4(positions[vertexId], 0.0f, 1.0f);
    output.Texcoord = texcoords[vertexId];
    return output;
}

float3 CubeFaceUvToDirection(uint faceIndex, float2 uv)
{
    const float2 p = uv * 2.0f - 1.0f;

    if (faceIndex == 0)
    {
        return normalize(float3(1.0f, -p.y, -p.x));
    }
    if (faceIndex == 1)
    {
        return normalize(float3(-1.0f, -p.y, p.x));
    }
    if (faceIndex == 2)
    {
        return normalize(float3(p.x, 1.0f, p.y));
    }
    if (faceIndex == 3)
    {
        return normalize(float3(p.x, -1.0f, -p.y));
    }
    if (faceIndex == 4)
    {
        return normalize(float3(p.x, -p.y, 1.0f));
    }

    return normalize(float3(-p.x, -p.y, -1.0f));
}

float2 DirectionToEquirectUv(float3 direction)
{
    direction = normalize(direction);
    const float u = atan2(direction.z, direction.x) / (2.0f * PI) + 0.5f;
    const float v = acos(clamp(direction.y, -1.0f, 1.0f)) / PI;
    return float2(u, v);
}

float4 PSMain(VSOutput input) : SV_TARGET
{
    const float3 direction = CubeFaceUvToDirection(FaceIndex, input.Texcoord);
    const float2 uv = DirectionToEquirectUv(direction);
    return float4(EquirectangularTexture.SampleLevel(LinearSampler, uv, 0.0f).rgb, 1.0f);
}
