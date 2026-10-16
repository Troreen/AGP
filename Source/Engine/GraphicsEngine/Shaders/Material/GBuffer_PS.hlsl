#include "Common.hlsli"
#include "Lighting.hlsli"
#include "Material.hlsli"
#include "MaterialParameters.hlsli"

Texture2D AlbedoTexture : register(t0);
Texture2D NormalTexture : register(t1);
Texture2D MaterialTexture : register(t2);
Texture2D FxTexture : register(t3);

struct GBufferOutput
{
    float4 AlbedoTexture : SV_TARGET0;
    float4 WorldNormalDepth : SV_TARGET1;
    float4 SurfaceTexture : SV_TARGET2;
    float4 Emission : SV_TARGET3;
    float4 WorldPosition : SV_TARGET4;
};

GBufferOutput main(VStoPS aPixel)
{
    const float4 sampleAlbedo = AlbedoTexture.Sample(TrilinearWrap, aPixel.UV0);
    const float2 sampleNormal = NormalTexture.Sample(TrilinearWrap, aPixel.UV0).xy;
    const float3 pixelNormal = GetWorldNormal(sampleNormal, aPixel.Normal, aPixel.Tangent, aPixel.BiNormal);
    const float4 sampleSurface = MaterialTexture.Sample(TrilinearWrap, aPixel.UV0);
    const float3 fxMap = FxTexture.Sample(TrilinearWrap, aPixel.UV0).rgb;
    
    const float emissionMask = fxMap.r;
	
    MaterialPixelParameters parameters;
    parameters.PixelColor = sampleAlbedo;
    parameters.SurfaceValues = sampleSurface;
    parameters.WorldPosition = aPixel.WorldPosition;
    parameters.UV0 = aPixel.UV0;
    parameters.Normal = pixelNormal;
    parameters.VertexNormal = aPixel.Normal;
    parameters.Tangent = aPixel.Tangent;
    parameters.BiNormal = aPixel.BiNormal;
    parameters.Emission = (float4)emissionMask * sampleAlbedo;
	
    Material_Pixel(parameters);
 
    GBufferOutput result;
    result.AlbedoTexture = parameters.PixelColor;
    switch (FB_RenderPassType)
    {
        case 7:
            result.WorldNormalDepth = float4(sampleNormal, 1, 1);
            break;
        
        default:
            result.WorldNormalDepth = float4(parameters.Normal, aPixel.ViewDepth);
            break;
    }
    result.SurfaceTexture = float4(sampleSurface.rgb, 0);
    switch (FB_RenderPassType)
    {
        case 5:
            result.Emission = float4(emissionMask, emissionMask, emissionMask, 1);
            break;
        
        default:
            result.Emission = parameters.Emission;
            break;
    }
    result.WorldPosition = parameters.WorldPosition;
    
    return result;
}
