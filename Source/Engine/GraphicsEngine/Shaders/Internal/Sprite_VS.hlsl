#include "../Material/Common.hlsli"

struct Vertex
{
    float4 Position : POSITION;
    float4 Color : COLOR;
    uint4 BoneIDs : BONEIDS;
    float4 SkinWeights : SKINWEIGHTS;
    float2 UV0 : UV0;
    float2 UV1 : UV1;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
};

VStoPS main(Vertex aVertex)
{
    VStoPS result;
    result.Position = mul(FB_Projection, aVertex.Position);
    result.UV0 = aVertex.UV0;
    result.ViewDepth = 0;
    result.Normal = 0;
    result.Binormal = 0;
    result.Color = 0;
    result.Tangent = 0;
    result.UV1 = 0;
    result.WorldPosition = 0;
	return result;
}