#include "MaterialParameters.hlsli"

cbuffer MaterialBuffer : register(b3)
{
    float4 MB_Tint = float4(1, 1, 1, 1);
}

void Material_Vertex(inout MaterialVertexParameters aParameters)
{
}

void Material_Pixel(inout MaterialPixelParameters aParameters)
{
    aParameters.PixelColor = float4(aParameters.UV0, 0, 1);
}