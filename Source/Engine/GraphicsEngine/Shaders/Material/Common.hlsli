#ifndef COMMON_HLSLI
#define COMMON_HLSLI

struct VStoPS
{
    float4 Position : SV_Position;
    float4 WorldPosition : WORLDPOS;
    float4 Color : COLOR;
    float2 UV0 : UV0;
    float2 UV1 : UV1;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
    float3 BiNormal : BINORMAL;
    float ViewDepth : VIEWDEPTH;
};

struct FullTextureVertex
{
    float4 Position : SV_Position;
    float2 UV : TEXCOORD0;
};

cbuffer FrameBuffer : register(b0)
{
    float4x4 FB_View;
    float4x4 FB_Projection;
    float3 FB_CameraPosition;
    float FB_TotalTime;
    uint FB_TonemapType;
    uint FB_RenderPassType;
    float2 __FB_padding;
    // The C++ frame buffer has the same extra row. Zero means the scene has no active sun.
    float3 FB_SunDirectionToLight;
    float __FB_SunPadding;
}

cbuffer ObjectBuffer : register(b1)
{
    float4x4 OB_World;
    float4x4 OB_WorldInvT;
    bool OB_HasSkinning;
    float3 __OB_Padding;
}

cbuffer AnimationBuffer : register(b2)
{
    float4x4 AB_JointTransforms[128];
}

#endif
