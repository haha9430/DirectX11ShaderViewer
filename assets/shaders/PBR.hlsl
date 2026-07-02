static const float PI = 3.14159265f;

cbuffer ObjectConstants : register(b0)
{
    matrix WorldViewProjection;
    matrix World;
    float3 CameraPosition;
    float Padding;
};

cbuffer MaterialConstants : register(b1)
{
    float4 BaseColorFactor;
    float MetallicFactor;
    float RoughnessFactor;
    float AoStrength;
    float NormalMapStrength;
    int DebugViewMode;
    float3 MaterialPadding;
};

cbuffer LightConstants : register(b2)
{
    float3 LightDirection;
    float LightIntensity;
    float3 LightColor;
    float DiffuseIblStrength;
    float3 SkyColor;
    float IblEnabled;
    float3 GroundColor;
    float EnvironmentMipCount;
    float SpecularIblStrength;
    float3 LightPadding;
};

Texture2D AlbedoTexture : register(t0);
Texture2D NormalTexture : register(t1);
Texture2D MetallicTexture : register(t2);
Texture2D RoughnessTexture : register(t3);
Texture2D AoTexture : register(t4);
TextureCube EnvironmentTexture : register(t5);
Texture2D BrdfLutTexture : register(t6);
TextureCube PrefilteredEnvironmentTexture : register(t7);
TextureCube IrradianceTexture : register(t8);
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
    float3 WorldPosition : TEXCOORD1;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
    float4 Color : COLOR;
    float2 Texcoord : TEXCOORD0;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    const float4 worldPosition = mul(float4(input.Position, 1.0f), World);
    output.Position = mul(float4(input.Position, 1.0f), WorldViewProjection);
    output.WorldPosition = worldPosition.xyz;
    output.Normal = normalize(mul(float4(input.Normal, 0.0f), World).xyz);
    output.Tangent = normalize(mul(float4(input.Tangent, 0.0f), World).xyz);
    output.Color = input.Color;
    output.Texcoord = input.Texcoord;
    return output;
}

float DistributionGGX(float3 normal, float3 halfway, float roughness)
{
    const float a = roughness * roughness;
    const float a2 = a * a;
    const float nDotH = max(dot(normal, halfway), 0.0f);
    const float nDotH2 = nDotH * nDotH;
    const float denominator = nDotH2 * (a2 - 1.0f) + 1.0f;
    return a2 / max(PI * denominator * denominator, 0.0001f);
}

float GeometrySchlickGGX(float nDotV, float roughness)
{
    const float r = roughness + 1.0f;
    const float k = (r * r) / 8.0f;
    return nDotV / max(nDotV * (1.0f - k) + k, 0.0001f);
}

float GeometrySmith(float3 normal, float3 viewDirection, float3 lightDirection, float roughness)
{
    const float nDotV = max(dot(normal, viewDirection), 0.0f);
    const float nDotL = max(dot(normal, lightDirection), 0.0f);
    return GeometrySchlickGGX(nDotV, roughness) * GeometrySchlickGGX(nDotL, roughness);
}

float3 FresnelSchlick(float cosTheta, float3 f0)
{
    return f0 + (1.0f - f0) * pow(saturate(1.0f - cosTheta), 5.0f);
}

float3 FresnelSchlickRoughness(float cosTheta, float3 f0, float roughness)
{
    return f0 + (max(float3(1.0f - roughness, 1.0f - roughness, 1.0f - roughness), f0) - f0) * pow(saturate(1.0f - cosTheta), 5.0f);
}

float3 SampleEnvironment(float3 direction, float mipLevel)
{
    return EnvironmentTexture.SampleLevel(LinearSampler, normalize(direction), mipLevel).rgb;
}

float3 ApproximateEnvironmentDiffuse(float3 normal)
{
    const float t = saturate(normal.y * 0.5f + 0.5f);
    const float3 fallbackColor = lerp(GroundColor, SkyColor, t);
    const float3 textureColor = IrradianceTexture.SampleLevel(LinearSampler, normalize(normal), 0.0f).rgb;
    return lerp(fallbackColor, textureColor, 0.85f) * DiffuseIblStrength * IblEnabled;
}

float3 ApproximateEnvironmentSpecular(float3 reflectionDirection, float roughness)
{
    const float maxReflectionMip = max(EnvironmentMipCount - 1.0f, 0.0f);
    const float mipLevel = saturate(roughness) * maxReflectionMip;
    const float3 textureColor = PrefilteredEnvironmentTexture.SampleLevel(LinearSampler, normalize(reflectionDirection), mipLevel).rgb;
    const float t = saturate(reflectionDirection.y * 0.5f + 0.5f);
    const float3 fallbackColor = lerp(GroundColor, SkyColor, t);
    const float3 envColor = lerp(fallbackColor, textureColor, 0.9f);
    return envColor * SpecularIblStrength * IblEnabled;
}

float3 SampleWorldNormal(PSInput input)
{
    const float3 vertexNormal = normalize(input.Normal);
    const float3 tangent = normalize(input.Tangent - vertexNormal * dot(input.Tangent, vertexNormal));
    const float3 bitangent = normalize(cross(vertexNormal, tangent));
    const float3x3 tangentToWorld = float3x3(tangent, bitangent, vertexNormal);

    float3 tangentNormal = NormalTexture.Sample(LinearSampler, input.Texcoord).xyz * 2.0f - 1.0f;
    tangentNormal.y *= -1.0f;
    const float3 mappedNormal = normalize(mul(tangentNormal, tangentToWorld));
    return normalize(lerp(vertexNormal, mappedNormal, NormalMapStrength));
}

float4 PSMain(PSInput input) : SV_TARGET
{
    const float3 textureAlbedoSrgb = AlbedoTexture.Sample(LinearSampler, input.Texcoord).rgb * input.Color.rgb;
    const float3 albedoSrgb = textureAlbedoSrgb * BaseColorFactor.rgb;
    const float3 albedo = pow(saturate(albedoSrgb), 2.2f);
    const float textureMetallic = MetallicTexture.Sample(LinearSampler, input.Texcoord).r;
    const float textureRoughness = RoughnessTexture.Sample(LinearSampler, input.Texcoord).r;
    const float textureAo = AoTexture.Sample(LinearSampler, input.Texcoord).r;
    const float metallic = saturate(textureMetallic * MetallicFactor);
    const float roughness = max(saturate(textureRoughness * RoughnessFactor), 0.045f);
    const float ao = lerp(1.0f, saturate(textureAo), AoStrength);

    const float3 normal = SampleWorldNormal(input);
    if (DebugViewMode == 1)
    {
        return float4(albedo, 1.0f);
    }
    if (DebugViewMode == 2)
    {
        return float4(normal * 0.5f + 0.5f, 1.0f);
    }
    if (DebugViewMode == 3)
    {
        return float4(metallic.xxx, 1.0f);
    }
    if (DebugViewMode == 4)
    {
        return float4(roughness.xxx, 1.0f);
    }
    if (DebugViewMode == 5)
    {
        return float4(ao.xxx, 1.0f);
    }

    const float3 viewDirection = normalize(CameraPosition - input.WorldPosition);
    const float3 lightDirection = normalize(LightDirection);
    const float3 halfway = normalize(viewDirection + lightDirection);
    const float3 radiance = LightColor * LightIntensity;

    const float ndf = DistributionGGX(normal, halfway, roughness);
    const float geometry = GeometrySmith(normal, viewDirection, lightDirection, roughness);
    const float3 f0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);
    const float3 fresnel = FresnelSchlick(max(dot(halfway, viewDirection), 0.0f), f0);

    const float3 numerator = ndf * geometry * fresnel;
    const float denominator = max(4.0f * max(dot(normal, viewDirection), 0.0f) * max(dot(normal, lightDirection), 0.0f), 0.0001f);
    const float3 specular = numerator / denominator;

    const float3 kS = fresnel;
    const float3 kD = (1.0f - kS) * (1.0f - metallic);
    const float nDotL = max(dot(normal, lightDirection), 0.0f);

    const float3 directLighting = (kD * albedo / PI + specular) * radiance * nDotL;
    if (DebugViewMode == 6)
    {
        return float4(directLighting, 1.0f);
    }

    const float3 ambientFresnel = FresnelSchlickRoughness(max(dot(normal, viewDirection), 0.0f), f0, roughness);
    const float3 ambientKs = ambientFresnel;
    const float3 ambientKd = (1.0f - ambientKs) * (1.0f - metallic);
    const float3 irradiance = ApproximateEnvironmentDiffuse(normal);
    const float3 diffuseIbl = irradiance * albedo;
    const float3 reflectionDirection = reflect(-viewDirection, normal);
    const float3 prefilteredColor = ApproximateEnvironmentSpecular(reflectionDirection, roughness);
    const float2 brdf = BrdfLutTexture.Sample(LinearSampler, float2(max(dot(normal, viewDirection), 0.0f), roughness)).rg;
    const float3 specularIbl = prefilteredColor * (ambientFresnel * brdf.x + brdf.y);
    if (DebugViewMode == 7)
    {
        return float4(ambientKd * diffuseIbl * ao, 1.0f);
    }
    if (DebugViewMode == 8)
    {
        return float4(specularIbl * ao, 1.0f);
    }
    if (DebugViewMode == 9)
    {
        return float4(irradiance, 1.0f);
    }
    if (DebugViewMode == 10)
    {
        return float4(prefilteredColor, 1.0f);
    }
    if (DebugViewMode == 11)
    {
        return float4(brdf.x, brdf.y, 0.0f, 1.0f);
    }

    const float3 ambient = (ambientKd * diffuseIbl + specularIbl) * ao;
    const float3 color = ambient + directLighting;
    return float4(color, 1.0f);
}
