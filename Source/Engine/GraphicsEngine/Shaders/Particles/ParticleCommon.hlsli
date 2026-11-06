#ifndef PARTICLE_COMMON_HLSLI
#define PARTICLE_COMMON_HLSLI

struct ParticleVertex
{
    float4 Position : POSITION;
    float4 Color : COLOR;
    float3 Velocity : VELOCITY;
    float Lifetime : LIFETIME;
    float Angle : ANGLE;
    float2 Size : SIZE;
    float4 ChannelMask : CHANNEL_MASK;
};

struct ParticleGStoPS
{
    float4 Position : SV_Position;
    float4 Color : COLOR;
    float3 Velocity : VELOCITY;
    float Lifetime : LIFETIME;
    float4 ChannelMask : CHANNEL_MASK;
    float2 UV : UV0;
};

#endif
