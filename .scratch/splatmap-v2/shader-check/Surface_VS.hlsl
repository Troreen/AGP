#include "Common.hlsli"
#include "Material.hlsli"

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
    [flatten]
    if (OB_HasSkinning)
    {
        float4x4 skinMatrix = 0;
        
        skinMatrix += AB_JointTransforms[aVertex.BoneIDs[0]] * aVertex.SkinWeights[0];
        skinMatrix += AB_JointTransforms[aVertex.BoneIDs[1]] * aVertex.SkinWeights[1];
        skinMatrix += AB_JointTransforms[aVertex.BoneIDs[2]] * aVertex.SkinWeights[2];
        skinMatrix += AB_JointTransforms[aVertex.BoneIDs[3]] * aVertex.SkinWeights[3];
        
        aVertex.Position = mul(skinMatrix, aVertex.Position);
        
        const float3x3 skinRotation = (float3x3) skinMatrix;
        aVertex.Normal = mul(skinRotation, aVertex.Normal);
        aVertex.Tangent = mul(skinRotation, aVertex.Tangent);
    }
    
    const float4 worldPos = mul(OB_World, aVertex.Position);
    
    MaterialVertexParameters parameters;
    parameters.WorldPosition = worldPos;
    parameters.VertexColor = aVertex.Color;
    parameters.UV0 = aVertex.UV0;
    parameters.UV1 = aVertex.UV1;
    Material_Vertex(parameters);
    
    const float4 viewPos = mul(FB_View, parameters.WorldPosition);
    const float4 clipPos = mul(FB_Projection, viewPos);
    
    VStoPS result;
    result.Position = clipPos;
    result.WorldPosition = worldPos;
    result.Color = parameters.VertexColor;
    result.UV0 = parameters.UV0;
    result.UV1 = parameters.UV1;
    
    const float3x3 worldNormalRot = (float3x3) OB_WorldInvT;
    result.Normal = normalize(mul(worldNormalRot, aVertex.Normal));
    result.Tangent = normalize(mul(worldNormalRot, aVertex.Tangent));
    result.BiNormal = normalize(cross(result.Normal, result.Tangent));
    
    result.ViewDepth = viewPos.z;
    
    return result;
}