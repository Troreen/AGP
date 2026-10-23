#ifndef MATERIAL_PARAMETERS_HLSLI
#define MATERIAL_PARAMETERS_HLSLI

struct MaterialVertexParameters
{
    float4 WorldPosition;
    float4 VertexColor;
    float2 UV0;
    float2 UV1;
};

struct MaterialPixelParameters
{
    float4 PixelColor;
    float4 VertexColor;
    float4 SurfaceValues;
    float4 WorldPosition;
    float2 UV0;
    float2 UV1;
    // Position on the render target in pixels, measured from its top-left corner.
    float2 ScreenPosition;
    float3 Normal;
    float3 VertexNormal;
    float3 Tangent;
    float3 BiNormal;
    float4 Emission;
};

#endif
