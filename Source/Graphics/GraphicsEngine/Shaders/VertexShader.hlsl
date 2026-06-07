#include "Common.hlsli"

struct Vertex
{
    float4 Position : POSITION;
    float4 Color : COLOR;
};

VStoPS main(Vertex aVertex)
{
    const float4 worldPos = mul(OB_World, aVertex.Position);
    const float4 viewPos = mul(FB_View, worldPos);
    const float4 clipPos = mul(FB_Projection, viewPos);

    VStoPS result;
    result.Position = clipPos;
    result.Color = aVertex.Color;
    return result;
}