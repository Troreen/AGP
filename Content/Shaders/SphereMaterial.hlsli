#include "MaterialParameters.hlsli"

cbuffer MaterialBuffer : register(b3)
{
    float4 MB_Tint = float4(0.9, 1.0, 0.92, 1);
}

void Material_Vertex(inout MaterialVertexParameters aParameters)
{
}

void Material_Pixel(inout MaterialPixelParameters aParameters)
{
    aParameters.PixelColor *= MB_Tint;
}
