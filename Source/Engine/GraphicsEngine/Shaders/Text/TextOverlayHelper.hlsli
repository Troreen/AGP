#ifndef TEXT_OVERLAY_HLSLI
#define TEXT_OVERLAY_HLSLI

struct VStoPS
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR;
    float2 UV : TEXCOORD0;
};

cbuffer TextOverlayBuffer : register(b6)
{
    float2 TB_ClientSize;
    float2 TB_PixelOrigin;
    float TB_Opacity;
    float TB_DistanceRange;
    float2 TB_AtlasSize;
};

#endif
