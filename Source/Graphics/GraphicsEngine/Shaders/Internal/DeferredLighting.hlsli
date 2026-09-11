#include "../Material/Common.hlsli"
#include "../Material/Samplers.hlsli"
#include "../Material/GlobalLightingTextures.hlsli"
#include "../Material/Lighting.hlsli"

Texture2D GBufferAlbedo : register(t0);
Texture2D GBufferNormal : register(t1);
Texture2D GBufferMaterial : register(t2);
Texture2D GBufferVertexNormal : register(t3);
Texture2D GBufferWorldPosition : register(t4);
Texture2D ScreenSpaceAO : register(t6);

struct FullTextureVertex
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD0;
};

bool GetDeferredSurface(FullTextureVertex aPixel, out float3 outDiffuse, out float3 outSpecular, out float outRoughness, out float outAO, out float3 outNormal, out float3 outPosition, out float3 outViewDir)
{
	// Initialize all out parameters before the no-geometry early exit.
	outDiffuse = 0.0f;
	outSpecular = 0.0f;
	outRoughness = 0.04f;
	outAO = 1.0f;
	outNormal = float3(0.0f, 1.0f, 0.0f);
	outPosition = 0.0f;
	outViewDir = float3(0.0f, 0.0f, 1.0f);
    const float4 albedo = GBufferAlbedo.Sample(TrilinearClamp, aPixel.UV);
    if (albedo.a == 0.0f)
        return false;

    const float3 material = GBufferMaterial.Sample(TrilinearClamp, aPixel.UV).rgb;
    const float metalness = saturate(material.b);
    outAO = saturate(material.r) * saturate(ScreenSpaceAO.Sample(TrilinearClamp, aPixel.UV).r);
    outRoughness = clamp(material.g, 0.04f, 1.0f);
    
    outNormal = normalize(GBufferNormal.Sample(TrilinearClamp, aPixel.UV).xyz);
    outPosition = GBufferWorldPosition.Sample(TrilinearClamp, aPixel.UV).xyz;
    const float3 albedoColor = saturate(albedo.rgb);
    outDiffuse = albedoColor * (1.0f - metalness);
    outSpecular = lerp((float3)0.04f, albedoColor, metalness);
    outViewDir = normalize(FB_CameraPosition - outPosition);
    return true;
}
