#include "DeferredLighting.hlsli"

float4 main(FullTextureVertex aPixel) : SV_TARGET
{
    float3 diffuse, specular, normal, position, viewDir;
    float roughness, ao;
    if (!GetDeferredSurface(aPixel, diffuse, specular, roughness, ao, normal, position, viewDir))
    {
        discard;
    }
    const Light light = LB_Lights[0];
    
    const float shadow = CalculateDirectionalShadow(light, position);
    const float3 radiance = CalculateDirectionalLight(light, diffuse, specular, roughness, normal, viewDir) * shadow;
    
    return float4(radiance, 1.0f);
}
