// Material textures stay in t0-t15. Global PBL textures sit immediately before
// the shadow map block, which starts at t100.
static const uint MATERIAL_ALBEDO_TEXTURE_SLOT = 0;
static const uint MATERIAL_NORMAL_TEXTURE_SLOT = 1;
static const uint MATERIAL_ORM_TEXTURE_SLOT = 2;
static const uint GLOBAL_ENV_CUBE_TEXTURE_SLOT = 98;
static const uint GLOBAL_BRDF_LUT_TEXTURE_SLOT = 99;
static const uint SHADOW_TEXTURE_SLOT_START = 100;

Texture2D AlbedoTexture : register(t0);
Texture2D NormalTexture : register(t1);
Texture2D MaterialTexture : register(t2);

float3 SampleTangentNormal(float2 aUV)
{
    float2 normalXY = NormalTexture.Sample(TrilinearWrap, aUV).rg * 2.0f - 1.0f;
    const float normalZ = sqrt(1.0f - saturate(dot(normalXY, normalXY)));
    return normalize(float3(normalXY, normalZ));
}

float3 TangentToWorldNormal(float3 aTangentNormal, float3 aNormal, float3 aTangent, float3 aBinormal)
{
    const float3x3 TBN = float3x3(normalize(aTangent), normalize(aBinormal), normalize(aNormal));
    return normalize(mul(aTangentNormal, TBN));
}

float3 SampleWorldNormal(float2 aUV, float3 aNormal, float3 aTangent, float3 aBinormal)
{
    return TangentToWorldNormal(SampleTangentNormal(aUV), aNormal, aTangent, aBinormal);
}

#include "GlobalLightingTextures.hlsli"
