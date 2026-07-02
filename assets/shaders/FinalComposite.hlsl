Texture2D SceneTexture : register(t0);
TextureCube EnvironmentTexture : register(t1);
SamplerState LinearSampler : register(s0);

cbuffer CompositeConstants : register(b0)
{
    matrix InverseViewProjection;
    float3 CameraPosition;
    float Exposure;
    float BackgroundIntensity;
    float ShowEnvironmentBackground;
    float2 Padding;
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

float3 AcesApprox(float3 color)
{
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    return saturate((color * (a * color + b)) / (color * (c * color + d) + e));
}

float3 ScreenUvToWorldDirection(float2 uv)
{
    const float2 clipXY = float2(uv.x * 2.0f - 1.0f, (1.0f - uv.y) * 2.0f - 1.0f);
    const float4 farClip = float4(clipXY, 1.0f, 1.0f);
    const float4 farWorld = mul(farClip, InverseViewProjection);
    const float3 worldPosition = farWorld.xyz / farWorld.w;
    return normalize(worldPosition - CameraPosition);
}

float4 PSMain(VSOutput input) : SV_TARGET
{
    const float4 scene = SceneTexture.Sample(LinearSampler, input.Texcoord);
    const float3 environmentDirection = ScreenUvToWorldDirection(input.Texcoord);
    const float3 environmentLinear = EnvironmentTexture.SampleLevel(LinearSampler, environmentDirection, 0.0f).rgb * BackgroundIntensity;
    const float3 background = ShowEnvironmentBackground > 0.5f ? environmentLinear : float3(0.0f, 0.0f, 0.0f);
    float3 hdrColor = lerp(background, scene.rgb, saturate(scene.a));
    hdrColor *= Exposure;

    float3 mapped = AcesApprox(hdrColor);
    mapped = pow(mapped, 1.0f / 2.2f);
    return float4(mapped, 1.0f);
}
