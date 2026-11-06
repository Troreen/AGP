#include "BloomHelper.hlsli"
#include "../Internal/Common.hlsli"
#include "../Internal/Samplers.hlsli"

float4 main(FullTextureVertex aInput) : SV_TARGET
{
    float4 bloomSample = Resource.Sample(LinearClamp, aInput.UV);
    return bloomSample * BB_Strength;
}