#include "PoissonDisks.hlsli"

static const uint SHADOW_SETTINGS_DEPTH_BIAS = 0;
static const uint SHADOW_POISSON_SAMPLE_COUNT = 16;
static const float SHADOW_MAP_TEXEL_SIZE = 1.0f / 2048.0f;
static const float SHADOW_POISSON_FILTER_RADIUS = 2.0f * SHADOW_MAP_TEXEL_SIZE;
static const float SHADOW_POISSON_SAMPLE_WEIGHT = 1.0f / 16.0f;

float GetShadowDepthBias(Light aLight)
{
    return aLight.ShadowSettings[SHADOW_SETTINGS_DEPTH_BIAS];
}

bool IsValidShadowCoord(float3 aShadowCoord)
{
    return aShadowCoord.x >= 0.0f && aShadowCoord.x <= 1.0f
        && aShadowCoord.y >= 0.0f && aShadowCoord.y <= 1.0f
        && aShadowCoord.z >= 0.0f && aShadowCoord.z <= 1.0f;
}

float SampleDirectionalShadowMap(uint aCascadeIndex, float2 aShadowUV, float aShadowDepth)
{
    float shadow = 1.0f;
    switch (aCascadeIndex)
    {
        case 0:
        {
            shadow = 0.0f;
            [unroll]
            for (uint sampleIndex = 0; sampleIndex < SHADOW_POISSON_SAMPLE_COUNT; ++sampleIndex)
            {
                const float2 sampleUV = aShadowUV + poissonDisk16[sampleIndex] * SHADOW_POISSON_FILTER_RADIUS;
                shadow += DirectionalShadowMaps[0].SampleCmpLevelZero(ShadowCmpSampler, sampleUV, aShadowDepth);
            }
            shadow *= SHADOW_POISSON_SAMPLE_WEIGHT;
            break;
        }
        case 1:
        {
            shadow = 0.0f;
            [unroll]
            for (uint sampleIndex = 0; sampleIndex < SHADOW_POISSON_SAMPLE_COUNT; ++sampleIndex)
            {
                const float2 sampleUV = aShadowUV + poissonDisk16[sampleIndex] * SHADOW_POISSON_FILTER_RADIUS;
                shadow += DirectionalShadowMaps[1].SampleCmpLevelZero(ShadowCmpSampler, sampleUV, aShadowDepth);
            }
            shadow *= SHADOW_POISSON_SAMPLE_WEIGHT;
            break;
        }
        case 2:
        {
            shadow = 0.0f;
            [unroll]
            for (uint sampleIndex = 0; sampleIndex < SHADOW_POISSON_SAMPLE_COUNT; ++sampleIndex)
            {
                const float2 sampleUV = aShadowUV + poissonDisk16[sampleIndex] * SHADOW_POISSON_FILTER_RADIUS;
                shadow += DirectionalShadowMaps[2].SampleCmpLevelZero(ShadowCmpSampler, sampleUV, aShadowDepth);
            }
            shadow *= SHADOW_POISSON_SAMPLE_WEIGHT;
            break;
        }
        case 3:
        {
            shadow = 0.0f;
            [unroll]
            for (uint sampleIndex = 0; sampleIndex < SHADOW_POISSON_SAMPLE_COUNT; ++sampleIndex)
            {
                const float2 sampleUV = aShadowUV + poissonDisk16[sampleIndex] * SHADOW_POISSON_FILTER_RADIUS;
                shadow += DirectionalShadowMaps[3].SampleCmpLevelZero(ShadowCmpSampler, sampleUV, aShadowDepth);
            }
            shadow *= SHADOW_POISSON_SAMPLE_WEIGHT;
            break;
        }
    }
    return shadow;
}

float SampleSpotShadowMap(uint aShadowIndex, float2 aShadowUV, float aShadowDepth)
{
    float shadow = 1.0f;
    switch (aShadowIndex)
    {
        case 0:
        {
            shadow = 0.0f;
            [unroll]
            for (uint sampleIndex = 0; sampleIndex < SHADOW_POISSON_SAMPLE_COUNT; ++sampleIndex)
            {
                const float2 sampleUV = aShadowUV + poissonDisk16[sampleIndex] * SHADOW_POISSON_FILTER_RADIUS;
                shadow += SpotLightShadowMaps[0].SampleCmpLevelZero(ShadowCmpSampler, sampleUV, aShadowDepth);
            }
            shadow *= SHADOW_POISSON_SAMPLE_WEIGHT;
            break;
        }
        case 1:
        {
            shadow = 0.0f;
            [unroll]
            for (uint sampleIndex = 0; sampleIndex < SHADOW_POISSON_SAMPLE_COUNT; ++sampleIndex)
            {
                const float2 sampleUV = aShadowUV + poissonDisk16[sampleIndex] * SHADOW_POISSON_FILTER_RADIUS;
                shadow += SpotLightShadowMaps[1].SampleCmpLevelZero(ShadowCmpSampler, sampleUV, aShadowDepth);
            }
            shadow *= SHADOW_POISSON_SAMPLE_WEIGHT;
            break;
        }
        case 2:
        {
            shadow = 0.0f;
            [unroll]
            for (uint sampleIndex = 0; sampleIndex < SHADOW_POISSON_SAMPLE_COUNT; ++sampleIndex)
            {
                const float2 sampleUV = aShadowUV + poissonDisk16[sampleIndex] * SHADOW_POISSON_FILTER_RADIUS;
                shadow += SpotLightShadowMaps[2].SampleCmpLevelZero(ShadowCmpSampler, sampleUV, aShadowDepth);
            }
            shadow *= SHADOW_POISSON_SAMPLE_WEIGHT;
            break;
        }
        case 3:
        {
            shadow = 0.0f;
            [unroll]
            for (uint sampleIndex = 0; sampleIndex < SHADOW_POISSON_SAMPLE_COUNT; ++sampleIndex)
            {
                const float2 sampleUV = aShadowUV + poissonDisk16[sampleIndex] * SHADOW_POISSON_FILTER_RADIUS;
                shadow += SpotLightShadowMaps[3].SampleCmpLevelZero(ShadowCmpSampler, sampleUV, aShadowDepth);
            }
            shadow *= SHADOW_POISSON_SAMPLE_WEIGHT;
            break;
        }
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

float CalculateProjectedShadow(Light aLight, uint aMatrixIndex, float3 aWorldPosition, bool aUseDirectionalMaps)
{
    float shadow = 1.0f;
    const float4 shadowPosition = mul(float4(aWorldPosition, 1.0f), aLight.LightViewProjTexture[aMatrixIndex]);
    const float3 shadowCoord = shadowPosition.xyz / shadowPosition.w;
    if (IsValidShadowCoord(shadowCoord))
    {
        const float shadowDepth = saturate(shadowCoord.z - GetShadowDepthBias(aLight));
        if (aUseDirectionalMaps)
        {
            shadow = SampleDirectionalShadowMap(aMatrixIndex, shadowCoord.xy, shadowDepth);
        }
        else
        {
            shadow = SampleSpotShadowMap(aLight.ShadowMapIndex, shadowCoord.xy, shadowDepth);
        }
    }

    return shadow;
}

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
