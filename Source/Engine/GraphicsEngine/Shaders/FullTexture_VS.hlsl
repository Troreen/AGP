#include "Material/Common.hlsli"

FullTextureVertex main(uint aVertexIndex : SV_VertexID)
{
    const float4 pos[4] =
    {
        float4(-1, -1, 0, 1),
        float4(-1, 1, 0, 1),
        float4(1, -1, 0, 1),
        float4(1, 1, 0, 1)
    };
    
    const float2 uv[4] =
    {
        float2(0.0f, 1.0f),
        float2(0.0f, 0.0f),
        float2(1.0f, 1.0f),
        float2(1.0f, 0.0f)
    };
    
    FullTextureVertex result;
    result.Position = pos[aVertexIndex];
    result.UV = uv[aVertexIndex];
	return result;
}