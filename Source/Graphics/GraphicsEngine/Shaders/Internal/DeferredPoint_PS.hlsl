#include "DeferredLighting.hlsli"

float4 main(FullTextureVertex aPixel) : SV_TARGET
{
    float3 diffuse, specular, normal, position, viewDir;
    float roughness, ao;
    if (!GetDeferredSurface(aPixel, diffuse, specular, roughness, ao, normal, position, viewDir)) discard;
    const Light light = LB_Lights[0];
    return float4(CalculatePointLight(light, diffuse, specular, roughness, normal, position, viewDir) * CalculatePointShadow(light, position), 1.0f);
}
