#ifndef BLOOM_HELPER_HLSLI
#define BLOOM_HELPER_HLSLI

Texture2D Resource : register(t0);

cbuffer BloomBuffer : register(b5)
{
    float BB_LuminanceExponent;
    float BB_LuminanceThreshhold;
    float2 __BB_padding;
}

#endif
