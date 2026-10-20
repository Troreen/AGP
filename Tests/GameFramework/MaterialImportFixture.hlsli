#include "MaterialParameters.hlsli"

cbuffer MaterialBuffer : register(b4)
{
    float MB_Glow_Amount = 0;
    float4 MB_Tint_Color = float4(0, 0, 0, 0);
};

Texture2D Noise_Map : register(t4);
Texture2D LegacyNoise : register(t5);

void Material_Vertex(inout MaterialVertexParameters aParameters)
{
}

void Material_Pixel(inout MaterialPixelParameters aParameters)
{
    aParameters.PixelColor *= MB_Tint_Color * MB_Glow_Amount;
    aParameters.PixelColor += Noise_Map.Load(int3(0, 0, 0));
    aParameters.PixelColor += LegacyNoise.Load(int3(0, 0, 0));
}
