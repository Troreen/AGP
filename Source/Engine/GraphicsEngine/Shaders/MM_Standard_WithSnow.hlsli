#include "MaterialParameters.hlsli"

cbuffer MaterialBuffer : register(b4)
{
    float4 MB_DiffuseColor = float4(1, 1, 1, 1);
    float MB_Roughness = 1;
    float MB_Metalness = 1;
    float MB_Tiling = 1;
    float __MB_padding;
    float4 MB_SnowColor = float4(0.95f, 0.97f, 1.0f, 1.0f);
    float MB_SnowAmount = 1.0f;
    float MB_SnowRoughness = 0.85f;
    float MB_SnowFlatness = 1.0f;
    float __MB_snowPadding;
}

void Material_Vertex(inout MaterialVertexParameters aParameters)
{
    aParameters.UV0 *= MB_Tiling;
}

void Material_Pixel(inout MaterialPixelParameters aParameters)
{
    aParameters.PixelColor *= MB_DiffuseColor;
    aParameters.SurfaceValues.g = saturate(aParameters.SurfaceValues.g * MB_Roughness);
    aParameters.SurfaceValues.b = saturate(aParameters.SurfaceValues.b * MB_Metalness);

    // Match the existing snow material's slope mask, retaining the rock normal's detail at the edge.
    const float3 rockNormal = normalize(aParameters.Normal);
    const float snowMask = saturate(rockNormal.y * 4.5f - 2.5f) * saturate(MB_SnowAmount);
    aParameters.PixelColor.rgb = lerp(aParameters.PixelColor.rgb, MB_SnowColor.rgb, snowMask);

    // Thick snow hides rock crevices and normal detail and behaves as a rough dielectric.
    aParameters.SurfaceValues.rgb = lerp(aParameters.SurfaceValues.rgb,
        float3(1.0f, saturate(MB_SnowRoughness), 0.0f), snowMask);
    aParameters.Normal = normalize(lerp(rockNormal, normalize(aParameters.VertexNormal),
        snowMask * saturate(MB_SnowFlatness)));
    aParameters.Emission.rgb *= 1.0f - snowMask;
}
