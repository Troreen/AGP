cbuffer FrameBuffer : register(b0)
{
    float4x4 FB_View;
    float4x4 FB_Projection;
    float3 FB_CameraPosition;
    float FB_TotalTime;
    uint FB_TonemapType;
    uint FB_RenderPassType;
    float2 __FB_padding;
}

struct Vertex
{
    float4 position : POSITION;
    float4 color : COLOR;
};

struct vsOutput
{

    float4 position : SV_Position;
    float4 color : COLOR;
};


vsOutput main(Vertex aVertex)
{
    float4 worldPos = aVertex.position;
    float4 viewPos = mul(FB_View, worldPos);
    float4 clipPos = mul(FB_Projection, viewPos);
    
    if (aVertex.color.a < 0)
    {
        aVertex.color.a = -aVertex.color.a;
        clipPos.z = 0e-5f;

    }
    
    vsOutput result;
    result.position = clipPos;
    result.color = aVertex.color;
    return result;
}