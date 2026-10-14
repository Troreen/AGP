#include "TextOverlayHelper.hlsli"

struct VertexInput
{
    float4 Position : POSITION;
    float4 Color : COLOR;
    uint4 BoneIDs : BONEIDS;
    float4 SkinWeights : SKINWEIGHTS;
    float2 UV : UV0;
    float2 UV1 : UV1;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
};

VStoPS main(VertexInput input)
{
    VStoPS output;
    float2 pixel = TB_PixelOrigin + input.Position.xy;
    output.Position = float4(pixel.x / TB_ClientSize.x * 2.0f - 1.0f, 1.0f - pixel.y / TB_ClientSize.y * 2.0f, 0.0f, 1.0f);
    output.Color = float4(input.Color.rgb, input.Color.a * TB_Opacity);
    output.UV = input.UV;
    return output;
}
