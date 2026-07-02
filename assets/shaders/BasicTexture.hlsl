cbuffer ObjectConstants : register(b0)
{
    matrix WorldViewProjection;
    matrix World;
};

Texture2D DiffuseTexture : register(t0);
Texture2D NormalTexture : register(t1);
SamplerState LinearSampler : register(s0);

struct VSInput
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
    float4 Color : COLOR;
    float2 Texcoord : TEXCOORD;
};

struct PSInput
{
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
    float4 Color : COLOR;
    float2 Texcoord : TEXCOORD;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.Position = mul(float4(input.Position, 1.0f), WorldViewProjection);
    output.Normal = normalize(mul(float4(input.Normal, 0.0f), World).xyz);
    output.Tangent = normalize(mul(float4(input.Tangent, 0.0f), World).xyz);
    output.Color = input.Color;
    output.Texcoord = input.Texcoord;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    const float3 lightDirection = normalize(float3(-0.35f, -1.0f, 0.25f));
    const float3 normal = normalize(input.Normal);
    const float3 tangent = normalize(input.Tangent - normal * dot(input.Tangent, normal));
    const float3 bitangent = normalize(cross(normal, tangent));
    const float3x3 tangentToWorld = float3x3(tangent, bitangent, normal);

    float3 normalSample = NormalTexture.Sample(LinearSampler, input.Texcoord).xyz * 2.0f - 1.0f;
    normalSample.y *= -1.0f;

    const float3 mappedNormal = normalize(mul(normalSample, tangentToWorld));
    const float diffuse = saturate(dot(mappedNormal, -lightDirection));
    const float lighting = 0.25f + diffuse * 0.75f;
    const float4 texel = DiffuseTexture.Sample(LinearSampler, input.Texcoord);
    return float4(texel.rgb * input.Color.rgb * lighting, texel.a * input.Color.a);
}
