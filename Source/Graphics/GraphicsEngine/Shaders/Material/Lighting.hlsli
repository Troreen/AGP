#include "PBR.hlsli"

static const uint LIGHT_TYPE_DIRECTIONAL = 0;
static const uint LIGHT_TYPE_POINT = 1;
static const uint LIGHT_TYPE_SPOT = 2;
static const uint MAX_LIGHTS = 8;

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

#include "ShadowSampling.hlsli"

float3 CalculateDirectionalLight(
    Light aLight,
    float3 aDiffuseColor,
    float3 aSpecularColor,
    float aRoughness,
    float3 aNormal,
    float3 aViewDir)
{
    const float3 lightDirection = normalize(-aLight.Direction);
    return CalculateDirectPBL(
        aLight.Color,
        aLight.Intensity,
        lightDirection,
        aDiffuseColor,
        aSpecularColor,
        aRoughness,
        aNormal,
        aViewDir);
}

float3 CalculatePointLight(
    Light aLight,
    float3 aDiffuseColor,
    float3 aSpecularColor,
    float aRoughness,
    float3 aNormal,
    float3 aWorldPosition,
    float3 aViewDir)
{
    const float3 toLight = aLight.Position - aWorldPosition;
    const float distanceToLight = max(length(toLight), 0.01f);
    const float3 lightDirection = toLight / distanceToLight;

    float rangeAttenuation = saturate(1.0f - (distanceToLight * distanceToLight) / (aLight.Radius * aLight.Radius));
    rangeAttenuation *= rangeAttenuation;

    const float illuminance = (aLight.Intensity / (distanceToLight * distanceToLight)) * rangeAttenuation;
    return CalculateDirectPBL(
        aLight.Color,
        illuminance,
        lightDirection,
        aDiffuseColor,
        aSpecularColor,
        aRoughness,
        aNormal,
        aViewDir);
}

float3 CalculateSpotLight(
    Light aLight,
    float3 aDiffuseColor,
    float3 aSpecularColor,
    float aRoughness,
    float3 aNormal,
    float3 aWorldPosition,
    float3 aViewDir)
{
    const float3 toLight = aLight.Position - aWorldPosition;
    const float distanceToLight = max(length(toLight), 0.01f);
    const float3 lightDirection = toLight / distanceToLight;

    float rangeAttenuation = saturate(1.0f - (distanceToLight * distanceToLight) / (aLight.Radius * aLight.Radius));
    rangeAttenuation *= rangeAttenuation;

    const float spotFactor = saturate(dot(-lightDirection, normalize(aLight.Direction)));
    const float spotAttenuation = smoothstep(cos(aLight.OuterCone), cos(aLight.InnerCone), spotFactor);
    const float illuminance = (aLight.Intensity / (distanceToLight * distanceToLight)) * rangeAttenuation * spotAttenuation;

    return CalculateDirectPBL(
        aLight.Color,
        illuminance,
        lightDirection,
        aDiffuseColor,
        aSpecularColor,
        aRoughness,
        aNormal,
        aViewDir);
}

float3 CalculateLighting(
    float3 aDiffuseColor,
    float3 aSpecularColor,
    float aRoughness,
    float3 aNormal,
    float3 aWorldPosition,
    float3 aViewDir)
{
    float3 radiance = 0.0f;
    const uint numLights = min(LB_NumActiveLights, MAX_LIGHTS);

    for (uint lightIndex = 0; lightIndex < numLights; ++lightIndex)
    {
        const Light light = LB_Lights[lightIndex];
        if (light.Type == LIGHT_TYPE_DIRECTIONAL)
        {
            radiance += CalculateDirectionalLight(light, aDiffuseColor, aSpecularColor, aRoughness, aNormal, aViewDir)
                * CalculateDirectionalShadow(light, aWorldPosition);
        }
        else if (light.Type == LIGHT_TYPE_POINT)
        {
            radiance += CalculatePointLight(light, aDiffuseColor, aSpecularColor, aRoughness, aNormal, aWorldPosition, aViewDir)
                * CalculatePointShadow(light, aWorldPosition);
        }
        else if (light.Type == LIGHT_TYPE_SPOT)
        {
            radiance += CalculateSpotLight(light, aDiffuseColor, aSpecularColor, aRoughness, aNormal, aWorldPosition, aViewDir)
                * CalculateSpotShadow(light, aWorldPosition);
        }
    }

    return radiance;
}
