#include "BloomHelper.hlsli"
#include "../Material/Common.hlsli"
#include "../Material/Lighting.hlsli"

float4 main(FullTextureVertex aInput) : SV_TARGET
{
    return Resource.Sample(TrilinearWrap, aInput.UV);
}