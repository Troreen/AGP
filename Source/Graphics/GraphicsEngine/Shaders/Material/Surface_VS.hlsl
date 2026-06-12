#include "Common.hlsli"
#include "Material.hlsli"

struct Vertex
{
    float4 Position : POSITION;
    float4 Color : COLOR;
    uint4 BoneIDs : BONEIDS;
    float4 SkinWeights : SKINWEIGHTS;
    float2 UV0 : TEXCOORD0;
    float2 UV1 : TEXCOORD1;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
};

VStoPS main(Vertex aVertex)
{
    float4 localPosition = aVertex.Position;
    float3 localNormal = aVertex.Normal;
    float3 localTangent = aVertex.Tangent;

    if (OB_HasSkinning)
    {
        float4x4 skinMatrix = 0;
        skinMatrix += AB_JointTransforms[aVertex.BoneIDs.x] * aVertex.SkinWeights.x;
        skinMatrix += AB_JointTransforms[aVertex.BoneIDs.y] * aVertex.SkinWeights.y;
        skinMatrix += AB_JointTransforms[aVertex.BoneIDs.z] * aVertex.SkinWeights.z;
        skinMatrix += AB_JointTransforms[aVertex.BoneIDs.w] * aVertex.SkinWeights.w;
        localPosition = mul(aVertex.Position, skinMatrix);

        const float3x3 skinRotation = (float3x3)skinMatrix;
        localNormal = mul(localNormal, skinRotation);
        localTangent = mul(localTangent, skinRotation);
    }

    const float4 worldPos = mul(localPosition, OB_World);
    const float3x3 worldNormalRot = (float3x3)OB_WorldInvT;
    const float3 worldNormal = normalize(mul(localNormal, worldNormalRot));
    const float3 worldTangent = normalize(mul(localTangent, worldNormalRot));
    const float3 worldBinormal = normalize(cross(worldNormal, worldTangent));

    MaterialVertexParameters parameters;
    parameters.WorldPosition = worldPos;
    parameters.VertexColor = aVertex.Color;
    parameters.UV0 = aVertex.UV0;
    parameters.UV1 = aVertex.UV1;
    parameters.Normal = worldNormal;
    parameters.Tangent = worldTangent;
    parameters.Binormal = worldBinormal;
    Material_Vertex(parameters);

    const float4 viewPos = mul(parameters.WorldPosition, FB_View);
    const float4 clipPos = mul(viewPos, FB_Projection);

    VStoPS result;
    result.Position = clipPos;
    result.Color = parameters.VertexColor;
    result.WorldPosition = parameters.WorldPosition;
    result.UV0 = parameters.UV0;
    result.UV1 = parameters.UV1;
    result.Normal = parameters.Normal;
    result.Tangent = parameters.Tangent;
    result.Binormal = parameters.Binormal;
    return result;
}
