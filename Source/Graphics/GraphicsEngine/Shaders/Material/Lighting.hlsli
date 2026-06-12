static const float PI = 3.14159265f;
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
    float2 __padding;
};

cbuffer LightBuffer : register(b4)
{
    Light LB_Lights[MAX_LIGHTS];
    uint LB_NumActiveLights;
    float3 __LB_padding;
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

float3 CalculateLighting(float3 anAlbedo, float3 aNormal, float3 aWorldPosition)
{
    float3 radiance = 0.0f;
    const uint numLights = min(LB_NumActiveLights, MAX_LIGHTS);

    for (uint lightIndex = 0; lightIndex < numLights; ++lightIndex)
    {
        const Light light = LB_Lights[lightIndex];
        if (light.Type == LIGHT_TYPE_DIRECTIONAL)
        {
            radiance += CalculateDirectionalLight(light, anAlbedo, aNormal);
        }
        else if (light.Type == LIGHT_TYPE_POINT)
        {
            radiance += CalculatePointLight(light, anAlbedo, aNormal, aWorldPosition);
        }
        else if (light.Type == LIGHT_TYPE_SPOT)
        {
            radiance += CalculateSpotLight(light, anAlbedo, aNormal, aWorldPosition);
        }
    }

    return radiance;
}
