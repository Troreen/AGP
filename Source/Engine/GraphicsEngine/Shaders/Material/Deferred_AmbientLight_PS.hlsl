#include "Common.hlsli"
#include "Lighting.hlsli"

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
	const float4 emissionMap = GBufferEmission.Sample(TrilinearClamp, input.UV);
    const float4 pixelWorldPos = GBufferWorldPosition.Sample(TrilinearClamp, input.UV);
	
    const float occlusion = materialMap.r;
    const float roughness = clamp(materialMap.g, 0.04f, 1.0f);
    const float metalness = materialMap.b;
	
    const float ssaoPixel = SSAOTexture.Sample(TrilinearClamp, input.UV).r;
    const float ambientOcclusion = min(occlusion, ssaoPixel);
    
    switch (FB_RenderPassType)
    {
        case 1: // Albedo Pass
            return float4(sampleAlbedo.rgb, 1);
        
        case 2: // Roughness Pass
            return float4(roughness, roughness, roughness, 1);
        
        case 3: // Metalness Pass
            return float4(metalness, metalness, metalness, 1);
        
        case 4: // Ambient Occlusion Texture Pass
            return float4(occlusion, occlusion, occlusion, 1);
        
        case 5: // Emission
            return float4(emissionMap.rrr, 1);
        
        case 6: // Ambient Occlusion Screen Space Pass
            return float4(ssaoPixel, ssaoPixel, ssaoPixel, 1);
        
        case 7: // Local Normal Pass
            return float4(pixelNormal, 1);
        
        case 8: // World Normal Pass
            return DebugNormal(pixelNormal);
    }
    
    const float3 specularColor = lerp((float3) 0.04f, sampleAlbedo.rgb, metalness);
    const float3 diffuseColor = lerp((float3) 0.00f, sampleAlbedo.rgb, 1 - metalness);
	
    float3 radiance = GetAmbientRadiance(specularColor, diffuseColor, pixelNormal, pixelWorldPos.xyz, roughness, ambientOcclusion);
    return float4(radiance + emissionMap.rgb, sampleAlbedo.a);
}
