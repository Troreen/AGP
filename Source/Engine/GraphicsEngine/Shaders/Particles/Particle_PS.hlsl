#include "ParticleCommon.hlsli"
#include "../Internal/Common.hlsli"
#include "../Internal/Samplers.hlsli"
#include "ParticleMaterial.hlsli"
#include "ParticleMaterialParameters.hlsli"

Texture2D ParticleTexture : register(t0);

float4 main(ParticleGStoPS aPixel) : SV_TARGET
{
    if (aPixel.Lifetime < 0.0f || FB_RenderPassType != 0)
    {
        discard;
    }
    
    const float4 textureColor = ParticleTexture.Sample(TrilinearWrap, aPixel.UV) * aPixel.ChannelMask;
    
    ParticleMaterialPixelParameters parameters;
    parameters.PixelColor   = float4(textureColor.rgb * textureColor.a, textureColor.a);
    parameters.VertexColor  = aPixel.Color;
    parameters.Position     = aPixel.Position;
    parameters.UV           = aPixel.UV;
	
    Particle_Pixel(parameters);
    
    return parameters.PixelColor * parameters.VertexColor;
}