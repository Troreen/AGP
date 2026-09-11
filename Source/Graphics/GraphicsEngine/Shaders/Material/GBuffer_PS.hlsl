#include "Common.hlsli"
#include "Samplers.hlsli"
#include "MaterialTextures.hlsli"
#include "Material.hlsli"
#include "MaterialParameters.hlsli"

struct GBufferOutput
{
    float4 Albedo : SV_TARGET0;
    float4 PixelNormal : SV_TARGET1;
    float4 Material : SV_TARGET2;
    float4 VertexNormal : SV_TARGET3;
    float4 WorldPosition : SV_TARGET4;
    float4 TangentNormal : SV_TARGET5;
};

GBufferOutput main(VStoPS aPixel)
{
    float4 albedo = AlbedoTexture.Sample(TrilinearWrap, aPixel.UV0) * aPixel.Color;
    const float3 pixelNormal = SampleWorldNormal(aPixel.UV0, aPixel.Normal, aPixel.Tangent, aPixel.Binormal);

    MaterialPixelParameters parameters;
    parameters.PixelColor = albedo;
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
    output.Material = float4(MaterialTexture.Sample(TrilinearWrap, parameters.UV0).rgb, 0.0f);
    output.VertexNormal = float4(normalize(aPixel.Normal), 0.0f);
    output.WorldPosition = parameters.WorldPosition;
    const float3 tangentNormal = float3(
        dot(parameters.Normal, normalize(aPixel.Tangent)),
        dot(parameters.Normal, normalize(aPixel.Binormal)),
        dot(parameters.Normal, normalize(aPixel.Normal)));
    output.TangentNormal = float4(normalize(tangentNormal), 0.0f);
    return output;
}
