#include "MaterialParameters.hlsli"

cbuffer MaterialBuffer : register(b4)
{
    float MB_Flatness = 0.0f;
}

void Material_Vertex(inout MaterialVertexParameters aParameters)
{
}

void Material_Pixel(inout MaterialPixelParameters aParameters)
{
    // FlattenNormal blends the tangent-space normal toward (0, 0, 1).
    // After the TBN transform, that endpoint is the unperturbed world vertex normal.
    aParameters.Normal = normalize(lerp(aParameters.Normal, normalize(aParameters.VertexNormal), MB_Flatness));

    // Unreal's blue world-normal channel is Z-up; AGP uses Y-up.
    const float snowMask = saturate(aParameters.Normal.y * 4.5f - 2.5f);
    const float3 baseColor = float3(0.05f, 0.05f, 0.05f);
    const float3 snowColor = float3(1.0f, 1.0f, 1.0f);
    aParameters.PixelColor.rgb = lerp(baseColor, snowColor, snowMask);
}
