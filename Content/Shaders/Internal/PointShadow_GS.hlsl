static const uint CUBE_FACE_COUNT = 6;
static const uint TRIANGLE_VERTEX_COUNT = 3;
static const uint MAX_OUTPUT_VERTEX_COUNT = CUBE_FACE_COUNT * TRIANGLE_VERTEX_COUNT;

cbuffer PointShadowBuffer : register(b5)
{
    row_major float4x4 PSB_ViewProjection[CUBE_FACE_COUNT];
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

[maxvertexcount(MAX_OUTPUT_VERTEX_COUNT)]
void main(triangle VStoPS input[TRIANGLE_VERTEX_COUNT], inout TriangleStream<PointShadowVertex> output)
{
    for (uint face = 0; face < CUBE_FACE_COUNT; ++face)
    {
        for (uint vertexIndex = 0; vertexIndex < TRIANGLE_VERTEX_COUNT; ++vertexIndex)
        {
            PointShadowVertex vertex;
            vertex.Position = mul(input[vertexIndex].WorldPosition, PSB_ViewProjection[face]);
            vertex.TargetIndex = face;
            output.Append(vertex);
        }

        output.RestartStrip();
    }
}
