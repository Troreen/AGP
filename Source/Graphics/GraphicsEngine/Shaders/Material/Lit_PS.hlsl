#include "Common.hlsli"
#include "Samplers.hlsli"
#include "MaterialTextures.hlsli"
#include "Lighting.hlsli"
#include "Material.hlsli"
#include "MaterialParameters.hlsli"

float4 main(VStoPS aPixel) : SV_TARGET
{
    float4 albedo = AlbedoTexture.Sample(TrilinearWrap, aPixel.UV0) * aPixel.Color;
    float3 pixelNormal = SampleWorldNormal(aPixel.UV0, aPixel.Normal, aPixel.Tangent, aPixel.Binormal);

    MaterialPixelParameters parameters;
    parameters.PixelColor = albedo;
    parameters.WorldPosition = aPixel.WorldPosition;
    parameters.UV0 = aPixel.UV0;
    parameters.UV1 = aPixel.UV1;
    parameters.Normal = pixelNormal;
    parameters.Tangent = aPixel.Tangent;
    parameters.Binormal = aPixel.Binormal;
    Material_Pixel(parameters);

    const float3 radiance = CalculateLighting(parameters.PixelColor.rgb, normalize(parameters.Normal), parameters.WorldPosition.xyz);
    return float4(LinearToGamma(radiance), parameters.PixelColor.a);
}
