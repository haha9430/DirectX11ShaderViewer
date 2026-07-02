static const float PI = 3.14159265f;
static const uint SAMPLE_COUNT = 128;

TextureCube EnvironmentTexture : register(t0);
SamplerState LinearSampler : register(s0);

cbuffer PrefilterConstants : register(b0)
{
    uint FaceIndex;
    float Roughness;
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

float RadicalInverseVdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10f;
}

float2 Hammersley(uint index, uint sampleCount)
{
    return float2(float(index) / float(sampleCount), RadicalInverseVdC(index));
}

float3 ImportanceSampleGGX(float2 xi, float roughness, float3 normal)
{
    const float a = roughness * roughness;
    const float phi = 2.0f * PI * xi.x;
    const float cosTheta = sqrt((1.0f - xi.y) / (1.0f + (a * a - 1.0f) * xi.y));
    const float sinTheta = sqrt(max(0.0f, 1.0f - cosTheta * cosTheta));

    const float3 halfwayTangent = float3(
        cos(phi) * sinTheta,
        sin(phi) * sinTheta,
        cosTheta);

    const float3 up = abs(normal.y) < 0.999f ? float3(0.0f, 1.0f, 0.0f) : float3(1.0f, 0.0f, 0.0f);
    const float3 tangent = normalize(cross(up, normal));
    const float3 bitangent = cross(normal, tangent);
    return normalize(tangent * halfwayTangent.x + bitangent * halfwayTangent.y + normal * halfwayTangent.z);
}

float4 PSMain(VSOutput input) : SV_TARGET
{
    const float3 normal = CubeFaceUvToDirection(FaceIndex, input.Texcoord);
    const float3 viewDirection = normal;

    if (Roughness <= 0.0001f)
    {
        return float4(EnvironmentTexture.SampleLevel(LinearSampler, normal, 0.0f).rgb, 1.0f);
    }

    float3 prefilteredColor = 0.0f.xxx;
    float totalWeight = 0.0f;

    [loop]
    for (uint i = 0; i < SAMPLE_COUNT; ++i)
    {
        const float2 xi = Hammersley(i, SAMPLE_COUNT);
        const float3 halfway = ImportanceSampleGGX(xi, Roughness, normal);
        const float3 lightDirection = normalize(2.0f * dot(viewDirection, halfway) * halfway - viewDirection);
        const float nDotL = max(dot(normal, lightDirection), 0.0f);

        if (nDotL > 0.0f)
        {
            prefilteredColor += EnvironmentTexture.SampleLevel(LinearSampler, lightDirection, 0.0f).rgb * nDotL;
            totalWeight += nDotL;
        }
    }

    prefilteredColor /= max(totalWeight, 0.0001f);
    return float4(prefilteredColor, 1.0f);
}
