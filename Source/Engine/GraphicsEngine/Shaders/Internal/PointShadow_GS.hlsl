cbuffer PointShadowBuffer : register(b5)
{
    row_major float4x4 PSB_ViewProjection[6];
}

struct VStoPS
{
    float4 Position : SV_Position;
    float4 Color : COLOR;
    float4 WorldPosition : WORLDPOS;
    float2 UV0 : UV0;
    float2 UV1 : UV1;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
    float3 Binormal : BINORMAL;
};

struct PointShadowVertex
{
    float4 Position : SV_Position;
    uint TargetIndex : SV_RenderTargetArrayIndex;
};

[maxvertexcount(18)]
void main(triangle VStoPS input[3], inout TriangleStream<PointShadowVertex> output)
{
    for (uint face = 0; face < 6; ++face)
    {
        for (uint vertexIndex = 0; vertexIndex < 3; ++vertexIndex)
        {
            PointShadowVertex vertex;
            vertex.Position = mul(input[vertexIndex].WorldPosition, PSB_ViewProjection[face]);
            vertex.TargetIndex = face;
            output.Append(vertex);
        }

        output.RestartStrip();
    }
}
