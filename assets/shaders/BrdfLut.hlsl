static const float PI = 3.14159265359f;
static const uint SAMPLE_COUNT = 1024u;

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

float RadicalInverseVdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10f;
}

float2 Hammersley(uint i, uint sampleCount)
{
    return float2(float(i) / float(sampleCount), RadicalInverseVdC(i));
}

float3 ImportanceSampleGGX(float2 xi, float roughness)
{
    const float a = roughness * roughness;
    const float phi = 2.0f * PI * xi.x;
    const float cosTheta = sqrt((1.0f - xi.y) / (1.0f + (a * a - 1.0f) * xi.y));
    const float sinTheta = sqrt(saturate(1.0f - cosTheta * cosTheta));

    return normalize(float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta));
}

float GeometrySchlickGGX(float nDotV, float roughness)
{
    const float a = roughness;
    const float k = (a * a) * 0.5f;
    return nDotV / max(nDotV * (1.0f - k) + k, 0.0001f);
}

float GeometrySmith(float nDotV, float nDotL, float roughness)
{
    return GeometrySchlickGGX(nDotV, roughness) * GeometrySchlickGGX(nDotL, roughness);
}

float2 IntegrateBrdf(float nDotV, float roughness)
{
    const float3 viewDirection = float3(sqrt(saturate(1.0f - nDotV * nDotV)), 0.0f, nDotV);
    float scale = 0.0f;
    float bias = 0.0f;

    [loop]
    for (uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        const float2 xi = Hammersley(i, SAMPLE_COUNT);
        const float3 halfwayDirection = ImportanceSampleGGX(xi, roughness);
        const float3 lightDirection = normalize(2.0f * dot(viewDirection, halfwayDirection) * halfwayDirection - viewDirection);

        const float nDotL = saturate(lightDirection.z);
        const float nDotH = saturate(halfwayDirection.z);
        const float vDotH = saturate(dot(viewDirection, halfwayDirection));

        if (nDotL > 0.0f)
        {
            const float geometry = GeometrySmith(nDotV, nDotL, roughness);
            const float geometryVisible = (geometry * vDotH) / max(nDotH * nDotV, 0.0001f);
            const float fresnel = pow(1.0f - vDotH, 5.0f);
            scale += (1.0f - fresnel) * geometryVisible;
            bias += fresnel * geometryVisible;
        }
    }

    return float2(scale, bias) / float(SAMPLE_COUNT);
}

float4 PSMain(VertexOutput input) : SV_TARGET
{
    const float nDotV = saturate(input.texcoord.x);
    const float roughness = saturate(input.texcoord.y);
    const float2 brdf = IntegrateBrdf(max(nDotV, 0.0001f), roughness);
    return float4(brdf, 0.0f, 1.0f);
}
