#include "BloomHelper.hlsli"
#include "../Material/Common.hlsli"
#include "../Material/Lighting.hlsli"

float4 main(FullTextureVertex aVertex) : SV_Target
{
    return Resource.Sample(TrilinearClamp, aVertex.UV);
}
