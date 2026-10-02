#include "MaterialParameters.hlsli"

cbuffer MaterialBuffer : register(b3)
{
    float4 MB_DiffuseColor = float4(1.0, 1.0, 1.0, 1);
    float MB_Roughness = 0;
    float MB_Metalness = 0;
    float2 __MB_padding;
}

void Material_Vertex(inout MaterialVertexParameters aParameters)
{
}

void Material_Pixel(inout MaterialPixelParameters aParameters)
{
    aParameters.PixelColor *= MB_DiffuseColor;
    aParameters.SurfaceValues.g += MB_Roughness;
    aParameters.SurfaceValues.b += MB_Metalness;
}
