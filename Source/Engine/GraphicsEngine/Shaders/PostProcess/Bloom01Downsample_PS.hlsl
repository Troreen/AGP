#include "BloomHelper.hlsli"
#include "../Internal/Common.hlsli"
#include "../Internal/Samplers.hlsli"

float4 main(FullTextureVertex aInput) : SV_TARGET
{
    return Resource.Sample(TrilinearClamp, aInput.UV);
}