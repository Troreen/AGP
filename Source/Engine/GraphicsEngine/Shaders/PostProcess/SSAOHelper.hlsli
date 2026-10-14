#ifndef SSAO_HELPER_HLSLI
#define SSAO_HELPER_HLSLI

#include "../Material/Common.hlsli"
#include "../Material/Lighting.hlsli"

cbuffer SSAOBuffer : register(b5)
{
    float SB_SampleRadiusTexels;
    float SB_MaxDistance;
    float SB_MinDistance;
    float __SB_padding;
}

static const float gWeights[11] =
{
    0.05f, 0.05f, 0.1f, 0.1f, 0.1f, 0.2f, 0.1f, 0.1f, 0.1f, 0.05f, 0.05f
};
static const int gBlurRadius = 5;

Texture2D Resource : register(t0);
Texture2D GBufferWorldNormalDepth : register(t1);
Texture2D GBufferWorldPosition : register(t4);

float4 SSAOBlur(FullTextureVertex aInput, float2 aTexOffset)
{
	// The center value always contributes to the sum.
    float4 color = gWeights[5] * Resource.SampleLevel(TrilinearClamp, aInput.UV, 0.0);
    float totalWeight = gWeights[5];
    float4 centerNormalDepth = GBufferWorldNormalDepth.SampleLevel(TrilinearClamp, aInput.UV, 0.0f);
	
    for (float i = -gBlurRadius; i <= gBlurRadius; ++i)
    {
		// We already added in the center weight.
        if (i == 0)
        {
            continue;
        }
		
        float2 tex = aInput.UV + i * aTexOffset;
        float4 neighborNormalDepth = GBufferWorldNormalDepth.SampleLevel(TrilinearClamp, tex, 0.0f);
		//
		// If the center value and neighbor values differ too
		// much (either in normal or depth), then we assume we
		// are sampling across a discontinuity. We discard
		// such samples from the blur.
		//
        if (dot(neighborNormalDepth.xyz, centerNormalDepth.xyz) >= 0.8f && abs(neighborNormalDepth.a - centerNormalDepth.a) <= 0.2f)
        {
            float weight = gWeights[i + gBlurRadius];
			// Add neighbor pixel to blur.
            color += weight * Resource.SampleLevel(TrilinearClamp, tex, 0.0);
            totalWeight += weight;
        }
    }
	
	// Compensate for discarded samples by making total weights sum to 1.
    return color / totalWeight;
}

#endif
