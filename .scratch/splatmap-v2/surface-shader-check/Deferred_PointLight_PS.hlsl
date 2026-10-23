#include "Common.hlsli"
#include "Lighting.hlsli"

#define LIGHT_INDEX 0

Texture2D GBufferAlbedo : register(t0);
Texture2D GBufferWorldNormal : register(t1);
Texture2D GBufferMaterial : register(t2);
Texture2D GBufferEmission : register(t3);
Texture2D GBufferWorldPosition : register(t4);

float4 main(FullTextureVertex input) : SV_TARGET
{
    const float4 sampleAlbedo = GBufferAlbedo.Sample(TrilinearClamp, input.UV);
	
	// If this is 0 we've read Infinity from the GBuffer. We cannot light Infinity so discard.
    if (sampleAlbedo.a < 0.05f)
        discard;
	
    const float3 pixelNormal = GBufferWorldNormal.Sample(TrilinearClamp, input.UV).xyz;
    const float4 materialMap = GBufferMaterial.Sample(TrilinearClamp, input.UV);
    const float4 pixelWorldPos = GBufferWorldPosition.Sample(TrilinearClamp, input.UV);
	
    const float occlusion = materialMap.r;
    const float roughness = clamp(materialMap.g, 0.04f, 1.0f);
    const float metalness = materialMap.b;
	
    const float3 specularColor = lerp((float3) 0.04f, sampleAlbedo.rgb, metalness);
    const float3 diffuseColor = lerp((float3) 0.00f, sampleAlbedo.rgb, 1 - metalness);
	
    uint shadowMapIndex = 0;
    float shadow = GetPointLightShadow(LIGHT_INDEX, pixelWorldPos, pixelNormal, shadowMapIndex);
    float3 radiance = CalculatePointLight(specularColor, diffuseColor, pixelWorldPos.xyz, pixelNormal, roughness, LB_Lights[LIGHT_INDEX]) * shadow;
	
    return float4(radiance, sampleAlbedo.a);
}