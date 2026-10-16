#include "Common.hlsli"
#include "Lighting.hlsli"
#include "Material.hlsli"
#include "MaterialParameters.hlsli"

Texture2D AlbedoTexture : register(t0);
Texture2D NormalTexture : register(t1);
Texture2D MaterialTexture : register(t2);
Texture2D FxTexture : register(t3);

float3 GetRadiance(float3 aSpecularColor, float3 aDiffuseColor, MaterialPixelParameters aPixel, float aSSAOPixel,
	float aOcclusion, float aRoughness, float aViewDepth, inout uint aPointLightShadowMapIndex, inout uint aSpotLightShadowMapIndex)
{
    const float3 pixelWorldPos = aPixel.WorldPosition.xyz;
    
    const float ambientOcclusion = min(aOcclusion, aSSAOPixel);
    float3 radiance = GetAmbientRadiance(aSpecularColor, aDiffuseColor, aPixel.Normal, pixelWorldPos, aRoughness, ambientOcclusion);
    
    for (uint l = 0; l < LB_NumActiveLights; ++l)
    {
        switch (LB_Lights[l].Type)
        {
            case 0:
			{
                float shadow = GetDirectionalLightShadow(l, aPixel.WorldPosition, aPixel.Normal, aViewDepth);
                radiance += CalculateDirectionalLight(aSpecularColor, aDiffuseColor, pixelWorldPos, aPixel.Normal, aRoughness, LB_Lights[l]) * shadow;
                break;
            }
			
            case 1:
			{
                float shadow = GetPointLightShadow(l, aPixel.WorldPosition, aPixel.Normal, aPointLightShadowMapIndex);
                radiance += CalculatePointLight(aSpecularColor, aDiffuseColor, pixelWorldPos, aPixel.Normal, aRoughness, LB_Lights[l]) * shadow;
                break;
            }
			
            case 2:
			{
                float shadow = GetSpotLightShadow(l, aPixel.WorldPosition, aPixel.Normal, aSpotLightShadowMapIndex);
                radiance += CalculateSpotLight(aSpecularColor, aDiffuseColor, pixelWorldPos, aPixel.Normal, aRoughness, LB_Lights[l]) * shadow;
                break;
            }
        }
    }
	
    return radiance;
}

float4 main(VStoPS aPixel) : SV_TARGET
{
	const float4 sampleAlbedo = AlbedoTexture.Sample(TrilinearWrap, aPixel.UV0);
	
	if (sampleAlbedo.a < 0.05f)
        discard;
	
	const float2 sampleNormal = NormalTexture.Sample(TrilinearWrap, aPixel.UV0).xy;
	const float3 pixelNormal = GetWorldNormal(sampleNormal, aPixel.Normal, aPixel.Tangent, aPixel.BiNormal);
	const float4 sampleSurface = MaterialTexture.Sample(TrilinearWrap, aPixel.UV0);
    const float3 fxMap = FxTexture.Sample(TrilinearWrap, aPixel.UV0).rgb;
	
    const float emissionMask = fxMap.r;
    
    const float ssaoPixel = SSAOTexture.Sample(TrilinearClamp, aPixel.UV0).r;
	
	MaterialPixelParameters parameters;
    parameters.PixelColor = sampleAlbedo;
    parameters.VertexColor = aPixel.Color;
    parameters.SurfaceValues = sampleSurface;
	parameters.WorldPosition = aPixel.WorldPosition;
	parameters.UV0 = aPixel.UV0;
    parameters.UV1 = aPixel.UV1;
	parameters.Normal = pixelNormal;
    parameters.VertexNormal = aPixel.Normal;
	parameters.Tangent = aPixel.Tangent;
	parameters.BiNormal = aPixel.BiNormal;
    parameters.Emission = (float4)emissionMask * sampleAlbedo;
    
	Material_Pixel(parameters);

    const float occlusion = parameters.SurfaceValues.r;
    const float roughness = clamp(parameters.SurfaceValues.g, 0.04f, 1.0f);
    const float metalness = parameters.SurfaceValues.b;
 
    switch (FB_RenderPassType)
    {
        case 1: // Albedo Pass
            return float4(parameters.PixelColor.rgb, 1);
        
        case 2: // Roughness Pass
            return float4(roughness, roughness, roughness, 1);
        
        case 3: // Metalness Pass
            return float4(metalness, metalness, metalness, 1);
        
        case 4: // Ambient Occlusion Texture Pass
            return float4(occlusion, occlusion, occlusion, 1);
        
        case 5: // Emission
            return float4(parameters.Emission.rgb, 1);
        
        case 6: // Ambient Occlusion Screen Space Pass
            return float4(ssaoPixel, ssaoPixel, ssaoPixel, 1);
        
        case 7: // Local Normal Pass
            return float4(sampleNormal, 1, 1);
        
        case 8: // World Normal Pass
            return DebugNormal(parameters.Normal);
    }
    
    uint pointLightShadowMapIndex = 0;
    uint spotLightShadowMapIndex = 0;
	
    const float3 specularColor = lerp((float3) 0.04f, parameters.PixelColor.rgb, metalness);
    const float3 diffuseColor = lerp((float3) 0.00f, parameters.PixelColor.rgb, 1 - metalness);
	
    float3 radiance = GetRadiance(specularColor, diffuseColor, parameters, ssaoPixel,
                                    occlusion, roughness, aPixel.ViewDepth, pointLightShadowMapIndex, spotLightShadowMapIndex);
    return float4(radiance + parameters.Emission.rgb, parameters.PixelColor.a);
}
