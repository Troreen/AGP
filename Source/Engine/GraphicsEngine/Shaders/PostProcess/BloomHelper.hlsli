#ifndef BLOOM_HELPER_HLSLI
#define BLOOM_HELPER_HLSLI

Texture2D Resource : register(t0);

cbuffer BloomBuffer : register(b5)
{
    float BB_FilterRadius;
    float BB_Strength;
    float BB_Threshold;
    float BB_Softness;
}

#endif
