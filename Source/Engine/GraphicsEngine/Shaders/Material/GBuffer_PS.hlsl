#include "Common.hlsli"
#include "Samplers.hlsli"
#include "MaterialTextures.hlsli"
#include "Material.hlsli"
#include "MaterialParameters.hlsli"

struct GBufferOutput
{
    float4 Albedo : SV_TARGET0;
    float4 PixelNormal : SV_TARGET1;
    float4 Surface : SV_TARGET2;
    float4 Emission : SV_TARGET3;
    float4 WorldPosition : SV_TARGET4;
    float4 TangentNormal : SV_TARGET5;
};

GBufferOutput main(VStoPS aPixel)
{
    const float4 albedo = AlbedoTexture.Sample(TrilinearWrap, aPixel.UV0) * aPixel.Color;
    const float4 surface = MaterialTexture.Sample(TrilinearWrap, aPixel.UV0);
    const float3 tangentNormal = SampleTangentNormal(aPixel.UV0);
    const float3 pixelNormal = TangentToWorldNormal(tangentNormal, aPixel.Normal, aPixel.Tangent, aPixel.Binormal);

    MaterialPixelParameters parameters;
    parameters.PixelColor = albedo;
    parameters.SurfaceValues = surface;
    parameters.WorldPosition = aPixel.WorldPosition;
    parameters.UV0 = aPixel.UV0;
    parameters.UV1 = aPixel.UV1;
    parameters.Normal = pixelNormal;
    parameters.Tangent = aPixel.Tangent;
    parameters.Binormal = aPixel.Binormal;
    Material_Pixel(parameters);

    GBufferOutput output;
    output.Albedo = float4(saturate(parameters.PixelColor.rgb), parameters.PixelColor.a);
    output.PixelNormal = float4(normalize(parameters.Normal), 0.0f);
    output.Surface = float4(parameters.SurfaceValues.rgb, 0.0f);
    // Emission is reserved for bloom and other post-process effects. Materials
    // do not expose emission yet, so initialize the future-facing buffer to 0.
    output.Emission = 0.0f;
    output.WorldPosition = parameters.WorldPosition;
    output.TangentNormal = float4(tangentNormal, 0.0f);
    return output;
}
