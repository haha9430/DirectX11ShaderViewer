TextureCube PreviewTexture : register(t0);
SamplerState LinearSampler : register(s0);

cbuffer CubePreviewConstants : register(b0)
{
    uint FaceIndex;
    float MipLevel;
    float2 Padding;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

VertexOutput VSMain(uint vertexId : SV_VertexID)
{
    VertexOutput output;
    const float2 positions[3] = {
        float2(-1.0f, -1.0f),
        float2(-1.0f, 3.0f),
        float2(3.0f, -1.0f),
    };

    const float2 position = positions[vertexId];
    output.position = float4(position, 0.0f, 1.0f);
    output.texcoord = float2(position.x * 0.5f + 0.5f, 0.5f - position.y * 0.5f);
    return output;
}

float3 DirectionFromFace(uint faceIndex, float2 uv)
{
    const float2 p = uv * 2.0f - 1.0f;

    if (faceIndex == 0u)
    {
        return normalize(float3(1.0f, -p.y, -p.x));
    }
    if (faceIndex == 1u)
    {
        return normalize(float3(-1.0f, -p.y, p.x));
    }
    if (faceIndex == 2u)
    {
        return normalize(float3(p.x, 1.0f, p.y));
    }
    if (faceIndex == 3u)
    {
        return normalize(float3(p.x, -1.0f, -p.y));
    }
    if (faceIndex == 4u)
    {
        return normalize(float3(p.x, -p.y, 1.0f));
    }

    return normalize(float3(-p.x, -p.y, -1.0f));
}

float3 AcesApprox(float3 color)
{
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    return saturate((color * (a * color + b)) / (color * (c * color + d) + e));
}

float4 PSMain(VertexOutput input) : SV_TARGET
{
    const float3 direction = DirectionFromFace(FaceIndex, input.texcoord);
    float3 color = PreviewTexture.SampleLevel(LinearSampler, direction, MipLevel).rgb;
    color = pow(AcesApprox(color), 1.0f / 2.2f);
    return float4(color, 1.0f);
}
