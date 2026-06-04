#include "Common.hlsli"

struct Vertex
{
    float4 Position : POSITION;
    float4 Color : COLOR;
};

VStoPS main(Vertex aVertex)
{
    VStoPS result;
    result.Position = aVertex.Position;
    result.Color = aVertex.Color;
    return result;
}