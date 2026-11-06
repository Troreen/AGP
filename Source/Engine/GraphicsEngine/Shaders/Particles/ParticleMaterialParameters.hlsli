#ifndef PARTICLE_MATERIAL_PARAMETERS_HLSLI
#define PARTICLE_MATERIAL_PARAMETERS_HLSLI

struct ParticleMaterialVertexParameters
{
    float4 WorldPosition;
    float4 VertexColor;
    float3 Velocity;
    float Lifetime;
    float Angle;
    float2 Size;
    float4 ChannelMask;
};

struct ParticleMaterialPixelParameters
{
    float4 PixelColor;
    float4 VertexColor;
    float4 Position;
    float2 UV;
};

#endif
