#ifndef M_PAINTED_LANDSCAPE_WEIGHTS_HLSLI
#define M_PAINTED_LANDSCAPE_WEIGHTS_HLSLI

#include "MaterialParameters.hlsli"

#ifndef LIGHTING_HLSLI
SamplerState TrilinearClamp : register(s3);
#endif

cbuffer MaterialBuffer : register(b4)
{
    float4 MB_MaskU;
    float4 MB_MaskV;
};

Texture2D SplatMask : register(t4);

void Material_Vertex(inout MaterialVertexParameters aParameters)
{
}

void Material_Pixel(inout MaterialPixelParameters aParameters)
{
    // Unreal ground XY is AGP ZX. Sample centres preserve the component borders.
    const float4 groundPosition = float4(aParameters.WorldPosition.zx, 0, 1);
    const float2 maskUV = float2(dot(MB_MaskU, groundPosition), dot(MB_MaskV, groundPosition));
    // Display the exported channels directly, including their original order.
    aParameters.PixelColor = float4(SplatMask.Sample(TrilinearClamp, maskUV).rgb, 1);
}

#endif
