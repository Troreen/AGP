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

cbuffer FrameBuffer : register(b0)
{
    row_major float4x4 FB_View;
    row_major float4x4 FB_Projection;
    float3 FB_CameraPosition;
    float __FB_Padding;
}

cbuffer ObjectBuffer : register(b1)
{
    row_major float4x4 OB_World;
    row_major float4x4 OB_WorldInvT;
    bool OB_HasSkinning;
    float3 __ob_Padding;
}

cbuffer AnimationBuffer : register(b2)
{
    row_major float4x4 AB_JointTransforms[128];
}
