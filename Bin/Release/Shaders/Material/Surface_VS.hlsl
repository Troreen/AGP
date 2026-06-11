#include "Common.hlsli"
#include "Material.hlsli"

struct Vertex
{
    float4 Position : POSITION;
    float4 Color : COLOR;
    uint4 BoneIDs : BONEIDS;
    float4 SkinWeights : SKINWEIGHTS;
};

VStoPS main(Vertex aVertex)
{
    float4 localPosition = aVertex.Position;

    if (OB_HasSkinning)
    {
        float4x4 skinMatrix = 0;
        skinMatrix += AB_JointTransforms[aVertex.BoneIDs.x] * aVertex.SkinWeights.x;
        skinMatrix += AB_JointTransforms[aVertex.BoneIDs.y] * aVertex.SkinWeights.y;
        skinMatrix += AB_JointTransforms[aVertex.BoneIDs.z] * aVertex.SkinWeights.z;
        skinMatrix += AB_JointTransforms[aVertex.BoneIDs.w] * aVertex.SkinWeights.w;
        localPosition = mul(aVertex.Position, skinMatrix);
    }

    const float4 worldPos = mul(localPosition, OB_World);

    MaterialVertexParameters parameters;
    parameters.WorldPosition = worldPos;
    parameters.VertexColor = aVertex.Color;
    Material_Vertex(parameters);

    const float4 viewPos = mul(parameters.WorldPosition, FB_View);
    const float4 clipPos = mul(viewPos, FB_Projection);

    VStoPS result;
    result.Position = clipPos;
    result.Color = parameters.VertexColor;
    return result;
}
