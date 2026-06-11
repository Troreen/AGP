struct VStoPS
{
    float4 Position : SV_Position;
    float4 Color : COLOR;
};

cbuffer FrameBuffer : register(b0)
{
    row_major float4x4 FB_View;
    row_major float4x4 FB_Projection;
}

cbuffer ObjectBuffer : register(b1)
{
    row_major float4x4 OB_World;
    bool OB_HasSkinning;
    float3 __ob_Padding;
}

cbuffer AnimationBuffer : register(b2)
{
    row_major float4x4 AB_JointTransforms[128];
}
