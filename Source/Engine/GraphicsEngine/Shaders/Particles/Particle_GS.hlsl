#include "ParticleCommon.hlsli"
#include "../Internal/Common.hlsli"

[maxvertexcount(4)]
void main(point ParticleVertex input[1], inout TriangleStream<ParticleGStoPS> output)
{
    const float2 offsets[4] =
    {
        { -1.0f, 1.0f },
        { 1.0f, 1.0f },
        { -1.0f, -1.0f },
        { 1.0f, -1.0f }
    };

    const float2 uvs[4] =
    {
        { 0.0f, 0.0f },
        { 1.0f, 0.0f },
        { 0.0f, 1.0f },
        { 1.0f, 1.0f }
    };
    const float PI = 3.14159265358979323846f;
    const float Deg2Rad = PI / 180.0f;
    
    const ParticleVertex vertex = input[0];
    for (uint i = 0; i < 4; ++i)
    {
        ParticleGStoPS result;
        
        result.Position = vertex.Position;
        const float4 origin = result.Position;
        result.Position.xy += offsets[i] * vertex.Size;
        result.UV = uvs[i];
        
        const float s = sin(vertex.Angle * Deg2Rad);
        const float c = cos(vertex.Angle * Deg2Rad);
        
        result.Position.x -= origin.x;
        result.Position.y -= origin.y;
        
        float x = result.Position.x * c - result.Position.y * s;
        float y = result.Position.x * s + result.Position.y * c;
        
        x += origin.x;
        y += origin.y;
        
        result.Position.xy = float2(x, y);
        result.Position = mul(FB_Projection, result.Position);
        
        result.Color = vertex.Color;
        result.Lifetime = vertex.Lifetime;
        result.Velocity = vertex.Velocity;
        result.ChannelMask = vertex.ChannelMask;
        
        output.Append(result);
    }
}