#ifndef MATERIAL_PARAMETERS_HLSLI
#define MATERIAL_PARAMETERS_HLSLI  

struct MaterialVertexParameters
{
    float4 WorldPosition;
    float4 VertexColor;
    float2 UV0;
    float2 UV1;
    float3 Normal;
    float3 Tangent;
    float3 Binormal;
};

struct MaterialPixelParameters
{
    float4 PixelColor;
    float4 WorldPosition;
    float2 UV0;
    float2 UV1;
    float3 Normal;
    float3 Tangent;
    float3 Binormal;
};

#endif 
