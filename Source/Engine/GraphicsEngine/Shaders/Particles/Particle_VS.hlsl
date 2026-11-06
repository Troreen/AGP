#include "ParticleCommon.hlsli"
#include "../Internal/Common.hlsli"
#include "ParticleMaterial.hlsli"
#include "ParticleMaterialParameters.hlsli"

ParticleVertex main(ParticleVertex aVertex)
{
    ParticleVertex result;
	
    const float4 worldPos = mul(OB_World, aVertex.Position);
	
    ParticleMaterialVertexParameters parameters;
    parameters.WorldPosition = worldPos;
    parameters.VertexColor   = aVertex.Color;
    parameters.Velocity      = aVertex.Velocity;
    parameters.Lifetime      = aVertex.Lifetime;
    parameters.Angle         = aVertex.Angle;
    parameters.Size          = aVertex.Size;
    parameters.ChannelMask   = aVertex.ChannelMask;
    
    Particle_Vertex(parameters);
    
    const float4 viewPos = mul(FB_View, parameters.WorldPosition);
    
    result.Position     = viewPos;
    result.Color        = parameters.VertexColor;
    result.Lifetime     = parameters.Lifetime;
    result.Velocity     = parameters.Velocity;
    result.Angle        = parameters.Angle;
    result.Size         = parameters.Size;
    result.ChannelMask  = parameters.ChannelMask;
	
	return result;
}
