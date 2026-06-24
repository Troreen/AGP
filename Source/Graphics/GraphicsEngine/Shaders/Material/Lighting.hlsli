#include "PoissonDisks.hlsli"

static const float PI = 3.14159265f;
static const uint LIGHT_TYPE_DIRECTIONAL = 0;
static const uint LIGHT_TYPE_POINT = 1;
static const uint LIGHT_TYPE_SPOT = 2;
static const uint MAX_LIGHTS = 8;
static const uint SHADOW_SETTINGS_DEPTH_BIAS = 0;
static const int SHADOW_PCF_SAMPLE_COUNT = 32;
static const float SHADOW_PCF_FILTER_RADIUS = 2.0f;

struct Light
{
    float3 Color;
    float Intensity;

    float3 Position;
    uint Type;

    float3 Direction;
    float InnerCone;

    float OuterCone;
    float Radius;
    uint ShadowMapIndex;
    uint NumCascades;

    row_major float4x4 LightViewProjTexture[4];
    float4 CascadeSplits;
    float4 ShadowSettings;
};

cbuffer LightBuffer : register(b4)
{
    Light LB_Lights[MAX_LIGHTS];
    uint LB_NumActiveLights;
    float3 __LB_padding;
}

float GetShadowDepthBias(Light aLight)
{
    return aLight.ShadowSettings[SHADOW_SETTINGS_DEPTH_BIAS];
}

float3 LinearToGamma(float3 aColor)
{
    return pow(abs(aColor), 1.0f / 2.2f);
}

float3 SampleWorldNormal(float2 aUV, float3 aNormal, float3 aTangent, float3 aBinormal)
{
    float2 normalXY = NormalTexture.Sample(TrilinearWrap, aUV).rg;
    normalXY = normalXY * 2.0f - 1.0f;

    const float normalZ = sqrt(1.0f - saturate(dot(normalXY, normalXY)));
    float3 tangentNormal = normalize(float3(normalXY, normalZ));

    const float3x3 TBN = float3x3(
        normalize(aTangent),
        normalize(aBinormal),
        normalize(aNormal)
    );

    return normalize(mul(tangentNormal, TBN));
}

float3 CalculateDirectionalLight(Light aLight, float3 anAlbedo, float3 aNormal)
{
    const float3 lightDirection = normalize(-aLight.Direction);
    const float NdotL = saturate(dot(aNormal, lightDirection));
    return NdotL * (anAlbedo / PI) * aLight.Color * aLight.Intensity;
}

float3 CalculatePointLight(Light aLight, float3 anAlbedo, float3 aNormal, float3 aWorldPosition)
{
    const float3 toLight = aLight.Position - aWorldPosition;
    const float distanceToLight = max(length(toLight), 0.01f);
    const float3 lightDirection = toLight / distanceToLight;

    float rangeAttenuation = saturate(1.0f - (distanceToLight * distanceToLight) / (aLight.Radius * aLight.Radius));
    rangeAttenuation *= rangeAttenuation;

    const float illuminance = (aLight.Intensity / (distanceToLight * distanceToLight)) * rangeAttenuation;
    const float NdotL = saturate(dot(aNormal, lightDirection));
    return NdotL * (anAlbedo / PI) * aLight.Color * illuminance;
}

float3 CalculateSpotLight(Light aLight, float3 anAlbedo, float3 aNormal, float3 aWorldPosition)
{
    const float3 toLight = aLight.Position - aWorldPosition;
    const float distanceToLight = max(length(toLight), 0.01f);
    const float3 lightDirection = toLight / distanceToLight;

    float rangeAttenuation = saturate(1.0f - (distanceToLight * distanceToLight) / (aLight.Radius * aLight.Radius));
    rangeAttenuation *= rangeAttenuation;

    const float spotFactor = saturate(dot(-lightDirection, normalize(aLight.Direction)));
    const float spotAttenuation = smoothstep(cos(aLight.OuterCone), cos(aLight.InnerCone), spotFactor);
    const float illuminance = (aLight.Intensity / (distanceToLight * distanceToLight)) * rangeAttenuation * spotAttenuation;

    const float NdotL = saturate(dot(aNormal, lightDirection));
    return NdotL * (anAlbedo / PI) * aLight.Color * illuminance;
}

bool IsValidShadowCoord(float3 aShadowCoord)
{
    return aShadowCoord.x >= 0.0f && aShadowCoord.x <= 1.0f
        && aShadowCoord.y >= 0.0f && aShadowCoord.y <= 1.0f
        && aShadowCoord.z >= 0.0f && aShadowCoord.z <= 1.0f;
}

float SampleShadowMapPCF(Texture2D aShadowMap, float2 aShadowUV, float aShadowDepth, float aShadowBias, float aFilterRadius)
{
    uint width = 0;
    uint height = 0;
    uint mipCount = 0;
    aShadowMap.GetDimensions(0, width, height, mipCount);

    const float2 texelSize = 1.0f / float2(width, height);
    const float comparisonDepth = saturate(aShadowDepth - aShadowBias);
    float shadow = 0.0f;

    [unroll]
    for (int sampleIndex = 0; sampleIndex < SHADOW_PCF_SAMPLE_COUNT; ++sampleIndex)
    {
        const float2 offset = poissonDisk32[sampleIndex] * texelSize * aFilterRadius;
        shadow += aShadowMap.SampleCmpLevelZero(ShadowCmpSampler, aShadowUV + offset, comparisonDepth);
    }

    return saturate(shadow / SHADOW_PCF_SAMPLE_COUNT);
}

float SampleDirectionalShadowMap(uint aCascadeIndex, float2 aShadowUV, float aShadowDepth, float aShadowBias)
{
    float shadow = 1.0f;
    switch (aCascadeIndex)
    {
        case 0:
            shadow = SampleShadowMapPCF(DirectionalShadowMaps[0], aShadowUV, aShadowDepth, aShadowBias, SHADOW_PCF_FILTER_RADIUS);
            break;
        case 1:
            shadow = SampleShadowMapPCF(DirectionalShadowMaps[1], aShadowUV, aShadowDepth, aShadowBias, SHADOW_PCF_FILTER_RADIUS);
            break;
        case 2:
            shadow = SampleShadowMapPCF(DirectionalShadowMaps[2], aShadowUV, aShadowDepth, aShadowBias, SHADOW_PCF_FILTER_RADIUS);
            break;
        case 3:
            shadow = SampleShadowMapPCF(DirectionalShadowMaps[3], aShadowUV, aShadowDepth, aShadowBias, SHADOW_PCF_FILTER_RADIUS);
            break;
    }
    return shadow;
}

float SampleSpotShadowMap(uint aShadowIndex, float2 aShadowUV, float aShadowDepth, float aShadowBias)
{
    float shadow = 1.0f;
    switch (aShadowIndex)
    {
        case 0:
            shadow = SampleShadowMapPCF(SpotLightShadowMaps[0], aShadowUV, aShadowDepth, aShadowBias, SHADOW_PCF_FILTER_RADIUS);
            break;
        case 1:
            shadow = SampleShadowMapPCF(SpotLightShadowMaps[1], aShadowUV, aShadowDepth, aShadowBias, SHADOW_PCF_FILTER_RADIUS);
            break;
        case 2:
            shadow = SampleShadowMapPCF(SpotLightShadowMaps[2], aShadowUV, aShadowDepth, aShadowBias, SHADOW_PCF_FILTER_RADIUS);
            break;
        case 3:
            shadow = SampleShadowMapPCF(SpotLightShadowMaps[3], aShadowUV, aShadowDepth, aShadowBias, SHADOW_PCF_FILTER_RADIUS);
            break;
    }
    return shadow;
}

float SamplePointShadowMap(uint aShadowIndex, float3 aDirection, float aShadowDepth)
{
    float shadow = 1.0f;
    switch (aShadowIndex)
    {
        case 0:
            shadow = PointLightShadowMaps[0].SampleCmpLevelZero(ShadowCmpSampler, aDirection, aShadowDepth);
            break;
        case 1:
            shadow = PointLightShadowMaps[1].SampleCmpLevelZero(ShadowCmpSampler, aDirection, aShadowDepth);
            break;
        case 2:
            shadow = PointLightShadowMaps[2].SampleCmpLevelZero(ShadowCmpSampler, aDirection, aShadowDepth);
            break;
        case 3:
            shadow = PointLightShadowMaps[3].SampleCmpLevelZero(ShadowCmpSampler, aDirection, aShadowDepth);
            break;
    }
    return shadow;
}

float CalculateProjectedShadow(Light aLight, uint aMatrixIndex, float3 aWorldPosition, bool aUseDirectionalMaps)
{
    float shadow = 1.0f;
    const float4 shadowPosition = mul(float4(aWorldPosition, 1.0f), aLight.LightViewProjTexture[aMatrixIndex]);
    const float3 shadowCoord = shadowPosition.xyz / shadowPosition.w;
    if (IsValidShadowCoord(shadowCoord))
    {
        const float shadowDepth = saturate(shadowCoord.z);
        const float shadowBias = GetShadowDepthBias(aLight);
        if (aUseDirectionalMaps)
        {
            shadow = SampleDirectionalShadowMap(aMatrixIndex, shadowCoord.xy, shadowDepth, shadowBias);
        }
        else
        {
            shadow = SampleSpotShadowMap(aLight.ShadowMapIndex, shadowCoord.xy, shadowDepth, shadowBias);
        }
    }

    return shadow;
}

uint GetDirectionalCascadeIndex(Light aLight, float aViewDepth);

float CalculateDirectionalShadow(Light aLight, float3 aWorldPosition)
{
    float shadow = 1.0f;
    const float viewDepth = mul(float4(aWorldPosition, 1.0f), FB_View).z;
    const uint cascadeIndex = GetDirectionalCascadeIndex(aLight, viewDepth);

    if (aLight.NumCascades > 0)
    {
        shadow = CalculateProjectedShadow(aLight, cascadeIndex, aWorldPosition, true);
    }

    return shadow;
}

float CalculateSpotShadow(Light aLight, float3 aWorldPosition)
{
    float shadow = 1.0f;
    if (aLight.NumCascades > 0)
    {
        shadow = CalculateProjectedShadow(aLight, 0, aWorldPosition, false);
    }

    return shadow;
}

float CalculatePointShadow(Light aLight, float3 aWorldPosition)
{
    float shadow = 1.0f;
    const float3 toPixel = aWorldPosition - aLight.Position;
    const float distanceToPixel = length(toPixel);
    if (aLight.NumCascades > 0 && distanceToPixel > 0.001f && distanceToPixel <= aLight.Radius)
    {
        const float3 absToPixel = abs(toPixel);
        const float z = max(absToPixel.x, max(absToPixel.y, absToPixel.z));
        const float nearPlane = 1.0f;
        const float farPlane = aLight.Radius;
        const float range = farPlane / (farPlane - nearPlane);
        const float depth = saturate(((range * z) - (range * nearPlane)) / z - GetShadowDepthBias(aLight));
        shadow = SamplePointShadowMap(aLight.ShadowMapIndex, normalize(toPixel), depth);
    }

    return shadow;
}

uint GetDirectionalCascadeIndex(Light aLight, float aViewDepth)
{
    uint cascadeIndex = 0;
    if (aViewDepth > aLight.CascadeSplits.z)
    {
        cascadeIndex = 3;
    }
    else if (aViewDepth > aLight.CascadeSplits.y)
    {
        cascadeIndex = 2;
    }
    else if (aViewDepth > aLight.CascadeSplits.x)
    {
        cascadeIndex = 1;
    }
    return cascadeIndex;
}

float3 CalculateLighting(float3 anAlbedo, float3 aNormal, float3 aWorldPosition)
{
    float3 radiance = 0.0f;
    const uint numLights = min(LB_NumActiveLights, MAX_LIGHTS);

    for (uint lightIndex = 0; lightIndex < numLights; ++lightIndex)
    {
        const Light light = LB_Lights[lightIndex];
        if (light.Type == LIGHT_TYPE_DIRECTIONAL)
        {
            radiance += CalculateDirectionalLight(light, anAlbedo, aNormal) * CalculateDirectionalShadow(light, aWorldPosition);
        }
        else if (light.Type == LIGHT_TYPE_POINT)
        {
            radiance += CalculatePointLight(light, anAlbedo, aNormal, aWorldPosition) * CalculatePointShadow(light, aWorldPosition);
        }
        else if (light.Type == LIGHT_TYPE_SPOT)
        {
            radiance += CalculateSpotLight(light, anAlbedo, aNormal, aWorldPosition) * CalculateSpotShadow(light, aWorldPosition);
        }
    }

    return radiance;
}
