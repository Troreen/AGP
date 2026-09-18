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

    const float3 materialMap = MaterialTexture.Sample(TrilinearWrap, parameters.UV0).rgb;
    const float ambientOcclusion = saturate(materialMap.r);
    const float roughness = clamp(materialMap.g, MIN_PBR_ROUGHNESS, 1.0f);
    const float metalness = saturate(materialMap.b);

    const float3 albedoColor = saturate(parameters.PixelColor.rgb);
    const float3 normal = normalize(parameters.Normal);
    const float3 viewDir = normalize(FB_CameraPosition - parameters.WorldPosition.xyz);
    const float3 diffuseColor = lerp((float3)0.0f, albedoColor, 1.0f - metalness);
    const float3 specularColor = lerp((float3)DIELECTRIC_SPECULAR, albedoColor, metalness);

    const float3 ambient = HasDirectionalLight()
        ? CalculateAmbientIBL(diffuseColor, specularColor, roughness, normal, viewDir, ambientOcclusion)
        : (float3)0.0f;
    const float3 directLighting = CalculateLighting(diffuseColor, specularColor, roughness, normal, parameters.WorldPosition.xyz, viewDir);
    const float3 finalColor = ambient + directLighting;

    return float4(LinearToGamma(finalColor), parameters.PixelColor.a);
}
