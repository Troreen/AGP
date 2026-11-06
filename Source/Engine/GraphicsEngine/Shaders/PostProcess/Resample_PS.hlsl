#include "BloomHelper.hlsli"
#include "../Internal/Common.hlsli"
#include "../Internal/Samplers.hlsli"

float4 main(FullTextureVertex aVertex) : SV_Target
{
    return Resource.Sample(TrilinearClamp, aVertex.UV);
}
