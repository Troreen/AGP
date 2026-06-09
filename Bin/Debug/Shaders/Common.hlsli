struct VStoPS
{
    float4 Position : SV_Position;
    float4 Color : COLOR;
};

cbuffer FrameBuffer : register(b0)
{
    float4x4 FB_View;
    float4x4 FB_Projection;
}

cbuffer ObjectBuffer : register(b1)
{
    float4x4 OB_World;
}