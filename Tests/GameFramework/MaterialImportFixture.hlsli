#include "MaterialParameters.hlsli"

struct ProjectionSettings
{
    float Weight;
    float4 Offset;
};

cbuffer MaterialBuffer : register(b4)
{
    float MB_Glow_Amount = 0;
    float4 MB_Tint_Color = float4(0, 0, 0, 0);
    float __Padding;
    float MB_Patch_Size = 800;
    float4 MB_Mask_Coverage;
    ProjectionSettings MB_Projection;
};

Texture2D Noise_Map : register(t4);
Texture2D LegacyNoise : register(t5);
Texture2D Dunes_BaseColor : register(t6);
Texture2D Rough_BaseColor : register(t7);
Texture2D Cliff_BaseColor : register(t8);

void Material_Vertex(inout MaterialVertexParameters aParameters)
{
}

void Material_Pixel(inout MaterialPixelParameters aParameters)
{
    aParameters.PixelColor *= MB_Tint_Color * MB_Glow_Amount;
    aParameters.PixelColor += MB_Mask_Coverage / MB_Patch_Size;
    aParameters.PixelColor += MB_Projection.Offset * MB_Projection.Weight;
    aParameters.PixelColor += Noise_Map.Load(int3(0, 0, 0));
    aParameters.PixelColor += LegacyNoise.Load(int3(0, 0, 0));
    aParameters.PixelColor += Dunes_BaseColor.Load(int3(0, 0, 0));
    aParameters.PixelColor += Rough_BaseColor.Load(int3(0, 0, 0));
    aParameters.PixelColor += Cliff_BaseColor.Load(int3(0, 0, 0));
}
