#include "DeferredLighting.hlsli"

float4 main(FullTextureVertex aPixel) : SV_TARGET
{
    float3 diffuse, specular, normal, position, viewDir;
    float roughness, ao;
    if (!GetDeferredSurface(aPixel, diffuse, specular, roughness, ao, normal, position, viewDir)) discard;
    const Light light = LB_Lights[0];
    const float shadow = CalculateDirectionalShadow(light, position);
    const float3 ambient = CalculateAmbientIBL(diffuse, specular, roughness, normal, viewDir, ao);
    const float3 direct = CalculateDirectionalLight(light, diffuse, specular, roughness, normal, viewDir) * shadow;
    return float4(ambient + direct, 1.0f);
}
