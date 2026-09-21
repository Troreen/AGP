#include "../Material/Samplers.hlsli"
#include "Tonemap.hlsli"

Texture2D HDRBuffer : register(t0);

cbuffer TonemapBuffer : register(b5)
{
    uint Tonemapper;
    uint TonemappingEnabled;
    float2 __Padding;
};

struct FullTextureVertex
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD0;
};

float4 main(FullTextureVertex aPixel) : SV_TARGET
{
    const float4 hdr = HDRBuffer.Sample(TrilinearClamp, aPixel.UV);
    const float3 linearColor = max(hdr.rgb, 0.0f);
    float3 outputColor;
    if (TonemappingEnabled == 0)
    {
        outputColor = LinearToGamma(linearColor);
    }
    else if (Tonemapper == 0)
    {
        outputColor = LinearToGamma(Tonemap_ACES(linearColor));
    }
    else if (Tonemapper == 1)
    {
        outputColor = LinearToGamma(Tonemap_Lottes(linearColor));
    }
    else
    {
        // Tonemap_UnrealEngine includes its own gamma conversion.
        outputColor = Tonemap_UnrealEngine(linearColor);
    }
    return float4(outputColor, 1.0f);
}