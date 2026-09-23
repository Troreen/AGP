#include "DeferredLighting.hlsli"

float4 main(FullTextureVertex aPixel) : SV_TARGET
{
    float3 diffuse, specular, normal, position, viewDir;
    float roughness, ao;
    if (!GetDeferredSurface(aPixel, diffuse, specular, roughness, ao, normal, position, viewDir))
    {
        discard;
    }
    
    const float3 radiance = CalculateAmbientIBL(diffuse, specular, roughness, normal, viewDir, ao);
    return float4(radiance, 1.0f);
}
