#ifndef LIGHTING_HLSLI
#define LIGHTING_HLSLI

#include "Common.hlsli"
#include "PoissonDisks.hlsli"

#define MAX_FORWARD_LIGHTS 8
#define NUM_CASCADES 3
#define PI 3.14159265358979323846f

struct Light
{
    float4x4 LightViewProjTexture[NUM_CASCADES];
	float3 Color;
	float Intensity;
	float3 Position;
	uint Type;
	float3 Direction;
	float InnerCone;
	float OuterCone;
	float Radius;
	bool CastShadows;
	float __padding;
};

cbuffer LightBuffer : register(b3)
{
	Light LB_Lights[MAX_FORWARD_LIGHTS];
	uint LB_NumActiveLights;
    float3 LB_CascadeSplits;
};

Texture2D SSAOTexture : register(t80);

TextureCube EnvironmentTexture : register(t98);
Texture2D BRDF_LUT_Texture : register(t99);

Texture2D DirLightShadowMaps[NUM_CASCADES] : register(t100);
Texture2D SpotLightShadowMaps[MAX_FORWARD_LIGHTS] : register(t110);
TextureCube PointLightShadowMaps[MAX_FORWARD_LIGHTS] : register(t120);

SamplerComparisonState ShadowCmpSampler : register(s0);
SamplerState LinearClamp : register(s1);
SamplerState TrilinearWrap : register(s2);
SamplerState TrilinearClamp : register(s3);

int GetNumMips(TextureCube aCube)
{
    int w = 0;
    int h = 0;
    int m = 0;
    aCube.GetDimensions(0, w, h, m);
    return m;
}

float3 GetAmbientRadiance(float3 aSpecularColor, float3 aDiffuseColor, float3 aPixelNormal, float3 aPixelWorldPos, float aRoughness, float aOcclusion)
{
    const int numMips = GetNumMips(EnvironmentTexture) - 1;
    const float3 iblDiffuse = EnvironmentTexture.SampleLevel(TrilinearWrap, aPixelNormal, numMips).rgb * 0.25f;
	
    const float3 directionToCam = normalize(FB_CameraPosition - aPixelWorldPos);
    const float3 reflection = reflect(-directionToCam, aPixelNormal);
    const float3 envColor = EnvironmentTexture.SampleLevel(TrilinearWrap, reflection, aRoughness * numMips).rgb * 0.25f;
	
    const float NdotV = saturate(dot(aPixelNormal, directionToCam));
    const float2 brdfLUT = BRDF_LUT_Texture.Sample(LinearClamp, float2(NdotV, aRoughness)).rg;
    const float3 iblSpecular = envColor * (aSpecularColor * brdfLUT.x + brdfLUT.y);

    return (aDiffuseColor * iblDiffuse + iblSpecular) * aOcclusion;
}

float NormalDistributionFunction_GGX(float aRoughness, float3 aPixelNormal, float3 aHalfAngle)
{
	const float alpha = aRoughness * aRoughness;
	const float alphaSqr = alpha * alpha;
	
	const float dotHalfAngle = saturate(dot(aPixelNormal, aHalfAngle));
	
	const float piMultiplier = ((dotHalfAngle * dotHalfAngle) * (alphaSqr - 1)) + 1;
	const float piMulSqr = piMultiplier * piMultiplier;
	
	const float denominator = PI * piMulSqr;
	
	return alphaSqr / denominator;
}

float3 Fresnel_SphericalGaussianSchlick(float3 aSpecularColor, float3 aDirectionToCamera, float3 aHalfAngle)
{
	const float dotCamHalfAngle = saturate(dot(aDirectionToCamera, aHalfAngle));
	const float p = ((-5.55473f * dotCamHalfAngle) - 6.98316f) * dotCamHalfAngle;
	
	return aSpecularColor + ((1.0f - aSpecularColor) * exp2(p));
}

float GeometricAttenuation_Smith(float aRoughness, float3 aPixelNormal, float3 aValue)
{
	const float k = ((aRoughness + 1) * (aRoughness + 1)) / 8.0f;
	const float dotNormal = saturate(dot(aPixelNormal, aValue));

	return dotNormal / (dotNormal * (1.0f - k) + k);
}

float GeometricAttenuation_Schlick_GGX(float aRoughness, float3 aPixelNormal, float3 aDirectionToLight, float3 aDirectionToCamera)
{
	return GeometricAttenuation_Smith(aRoughness, aPixelNormal, aDirectionToLight) * GeometricAttenuation_Smith(aRoughness, aPixelNormal, aDirectionToCamera);
}

float3 Specular_BRDF(float aRoughness, float3 aPixelNormal, float3 aDirectionToCam, float3 aDirectionToLight, float3 aSpecularColor)
{
	const float3 combinedDirection = aDirectionToLight + aDirectionToCam;
	const float3 halfAngle = combinedDirection / length(combinedDirection);
	
	const float dotLight = dot(aPixelNormal, aDirectionToLight);
	const float dotCam = dot(aPixelNormal, aDirectionToCam);
	const float denominator = 4 * dotLight * dotCam;
	
	const float3 ks = (
			NormalDistributionFunction_GGX(aRoughness, aPixelNormal, halfAngle)
			* Fresnel_SphericalGaussianSchlick(aSpecularColor, aDirectionToCam, halfAngle)
			* GeometricAttenuation_Schlick_GGX(aRoughness, aPixelNormal, aDirectionToLight, aDirectionToCam)
		) / denominator;
	
	return ks;
}

float3 Diffuse_BRDF(float3 aDiffuseColor)
{
	return aDiffuseColor / PI;
}

float3 CalculateDirectionalLight(float3 aSpecularColor, float3 aDiffuseColor, float3 aPixelWorldPos, float3 aPixelNormal, float aRoughness, Light aLight)
{
	const float3 directionToCam = normalize(FB_CameraPosition - aPixelWorldPos);
	const float3 directionToLight = normalize(-aLight.Direction);
	
	const float3 ks = Specular_BRDF(aRoughness, aPixelNormal, directionToCam, directionToLight, aSpecularColor);
	float3 kd = Diffuse_BRDF(aDiffuseColor);
	kd *= (1.0f - ks);
	
	const float3 lightColor = aLight.Color * aLight.Intensity;
	const float nDotL = saturate(dot(directionToLight, aPixelNormal));
	return (kd + ks) * lightColor * nDotL;
}

float3 CalculatePointLight(float3 aSpecularColor, float3 aDiffuseColor, float3 aPixelWorldPos, float3 aPixelNormal, float aRoughness, Light aLight)
{
	const float3 directionToCam = normalize(FB_CameraPosition - aPixelWorldPos);
	const float3 directionToLight = normalize(aLight.Position - aPixelWorldPos);
	
	const float3 ks = Specular_BRDF(aRoughness, aPixelNormal, directionToCam, directionToLight, aSpecularColor);
	float3 kd = Diffuse_BRDF(aDiffuseColor);
	kd *= (1.0f - ks);
	
	const float distanceToLight = max(length(aLight.Position - aPixelWorldPos), 0.01f);
	const float safeRadius = max(aLight.Radius, 0.01f);
	const float distanceRelation = pow(distanceToLight / safeRadius, 2.0f);
	
	float alpha = saturate(1.0f - distanceRelation);
	alpha *= alpha;
	
	const float lightIntensity = (aLight.Intensity / pow(distanceToLight, 2.0f)) * alpha;
	const float3 lightColor = aLight.Color * lightIntensity;
	const float nDotL = saturate(dot(directionToLight, aPixelNormal));
	return (kd + ks) * lightColor * nDotL;
}

float3 CalculateSpotLight(float3 aSpecularColor, float3 aDiffuseColor, float3 aPixelWorldPos, float3 aPixelNormal, float aRoughness, Light aLight)
{
	const float3 directionToCam = normalize(FB_CameraPosition - aPixelWorldPos);
	const float3 directionToLight = normalize(aLight.Position - aPixelWorldPos);
	
	const float3 ks = Specular_BRDF(aRoughness, aPixelNormal, directionToCam, directionToLight, aSpecularColor);
	float3 kd = Diffuse_BRDF(aDiffuseColor);
	kd *= (1.0f - ks);
	
	const float cosInner = cos(aLight.InnerCone);
	const float cosOuter = cos(aLight.OuterCone);
	
	const float spotlightFactor = saturate(dot(-directionToLight, aLight.Direction));
	const float spotAttenuation = smoothstep(cosOuter, cosInner, spotlightFactor);
	
	const float distanceToLight = max(length(aLight.Position - aPixelWorldPos), 0.01f);
	const float safeRadius = max(aLight.Radius, 0.01f);
	const float distanceRelation = pow(distanceToLight / safeRadius, 2.0f);
	
	float alpha = saturate(1.0f - distanceRelation);
	alpha *= alpha;
	alpha *= spotAttenuation;
	
	const float lightIntensity = (aLight.Intensity / pow(distanceToLight, 2.0f)) * alpha;
	const float3 lightColor = aLight.Color * lightIntensity;
	const float nDotL = saturate(dot(directionToLight, aPixelNormal));
	return (kd + ks) * lightColor * nDotL;
}

float3 GetWorldNormal(float2 aSampleNormal, float3 aNormal, float3 aTangent, float3 aBiNormal)
{
	aSampleNormal.x = (aSampleNormal.x * 2) - 1;
	aSampleNormal.y = (aSampleNormal.y * 2) - 1;
	
	const float xSqr = aSampleNormal.x * aSampleNormal.x;
	const float ySqr = aSampleNormal.y * aSampleNormal.y;
	const float subSqrt = 1 - saturate(xSqr + ySqr);
	const float z = sqrt(subSqrt);
	
	float3 worldNormal = normalize(float3(aSampleNormal, z));
	
	const float3x3 TBN = float3x3(
		normalize(aTangent),
		normalize(aBiNormal),
		normalize(aNormal)
	);
	
	return mul(worldNormal, TBN);
}

float4 DebugNormal(float3 aNormal)
{
	float3 debugNormal = (aNormal + 1.0f) * 0.5f;
	return float4(debugNormal, 1);
}

void GetCascadeShadowMapDimensions(uint aIndex, inout uint width, inout uint height, inout uint mips)
{
    width = height = mips = 0;
	
    switch (aIndex)
    {
        case 0:
            DirLightShadowMaps[0].GetDimensions(0, width, height, mips);
			break;
        case 1:
            DirLightShadowMaps[1].GetDimensions(0, width, height, mips);
			break;
        case 2:
            DirLightShadowMaps[2].GetDimensions(0, width, height, mips);
			break;
    }
}

float SampleCascadeShadowMap(uint aIndex, float2 aShadowUV, float aShadowDepth)
{
    switch (aIndex)
    {
        case 0:
            return DirLightShadowMaps[0].SampleCmpLevelZero(ShadowCmpSampler, aShadowUV, aShadowDepth);
        case 1:
            return DirLightShadowMaps[1].SampleCmpLevelZero(ShadowCmpSampler, aShadowUV, aShadowDepth);
        case 2:
            return DirLightShadowMaps[2].SampleCmpLevelZero(ShadowCmpSampler, aShadowUV, aShadowDepth);
        default:
            return 1;
    }
}

float SampleSpotLightShadow(uint aIndex, float2 aShadowUV, float aShadowDepth)
{
	switch (aIndex)
	{
		case 0:
			return SpotLightShadowMaps[0].SampleCmpLevelZero(ShadowCmpSampler, aShadowUV, aShadowDepth);
		case 1:
			return SpotLightShadowMaps[1].SampleCmpLevelZero(ShadowCmpSampler, aShadowUV, aShadowDepth);
		case 2:
			return SpotLightShadowMaps[2].SampleCmpLevelZero(ShadowCmpSampler, aShadowUV, aShadowDepth);
		case 3:
			return SpotLightShadowMaps[3].SampleCmpLevelZero(ShadowCmpSampler, aShadowUV, aShadowDepth);
		case 4:
			return SpotLightShadowMaps[4].SampleCmpLevelZero(ShadowCmpSampler, aShadowUV, aShadowDepth);
		case 5:
			return SpotLightShadowMaps[5].SampleCmpLevelZero(ShadowCmpSampler, aShadowUV, aShadowDepth);
		case 6:
			return SpotLightShadowMaps[6].SampleCmpLevelZero(ShadowCmpSampler, aShadowUV, aShadowDepth);
		case 7:
			return SpotLightShadowMaps[7].SampleCmpLevelZero(ShadowCmpSampler, aShadowUV, aShadowDepth);
		default:
			return 1;
	}
}

float SamplePointLightShadow(uint aIndex, float3 aDirection, float aShadowDepth)
{
	switch (aIndex)
	{
		case 0:
			return PointLightShadowMaps[0].SampleCmpLevelZero(ShadowCmpSampler, aDirection, aShadowDepth);
		case 1:
			return PointLightShadowMaps[1].SampleCmpLevelZero(ShadowCmpSampler, aDirection, aShadowDepth);
		case 2:
			return PointLightShadowMaps[2].SampleCmpLevelZero(ShadowCmpSampler, aDirection, aShadowDepth);
		case 3:
			return PointLightShadowMaps[3].SampleCmpLevelZero(ShadowCmpSampler, aDirection, aShadowDepth);
		case 4:
			return PointLightShadowMaps[4].SampleCmpLevelZero(ShadowCmpSampler, aDirection, aShadowDepth);
		case 5:
			return PointLightShadowMaps[5].SampleCmpLevelZero(ShadowCmpSampler, aDirection, aShadowDepth);
		case 6:
			return PointLightShadowMaps[6].SampleCmpLevelZero(ShadowCmpSampler, aDirection, aShadowDepth);
		case 7:
			return PointLightShadowMaps[7].SampleCmpLevelZero(ShadowCmpSampler, aDirection, aShadowDepth);
		default:
			return 1;
	}
}

uint GetSpotLightShadowMapWidth(uint aIndex, uint aMipLevel)
{
	uint shadowWidth, shadowHeight, shadowMips;
	switch (aIndex)
	{
		case 0:
			SpotLightShadowMaps[0].GetDimensions(aMipLevel, shadowWidth, shadowHeight, shadowMips);
			break;
		case 1:
			SpotLightShadowMaps[1].GetDimensions(aMipLevel, shadowWidth, shadowHeight, shadowMips);
			break;
		case 2:
			SpotLightShadowMaps[2].GetDimensions(aMipLevel, shadowWidth, shadowHeight, shadowMips);
			break;
		case 3:
			SpotLightShadowMaps[3].GetDimensions(aMipLevel, shadowWidth, shadowHeight, shadowMips);
			break;
		case 4:
			SpotLightShadowMaps[4].GetDimensions(aMipLevel, shadowWidth, shadowHeight, shadowMips);
			break;
		case 5:
			SpotLightShadowMaps[5].GetDimensions(aMipLevel, shadowWidth, shadowHeight, shadowMips);
			break;
		case 6:
			SpotLightShadowMaps[6].GetDimensions(aMipLevel, shadowWidth, shadowHeight, shadowMips);
			break;
		case 7:
			SpotLightShadowMaps[7].GetDimensions(aMipLevel, shadowWidth, shadowHeight, shadowMips);
			break;
		default:
			shadowWidth = 0;
			break;
	}
	
	return shadowWidth;
}

static const float factor = 0.00001f;
static const float minBias = 0.00002f;
static const float maxBias = 0.01f;

int GetDirectionalCascadeIndex(float aViewDepth)
{
    for (int i = 0; i < NUM_CASCADES; ++i)
    {
        if (aViewDepth < LB_CascadeSplits[i])
        {
            return i;
        }
    }
	
    return NUM_CASCADES - 1;
}

float GetDirectionalLightShadow(uint aLightIndex, float4 aPixelWorldPosition, float3 aPixelNormal, float aViewDepth)
{
    if (LB_Lights[aLightIndex].CastShadows)
    {
        int cascadeIndex = GetDirectionalCascadeIndex(aViewDepth);
        uint shadowWidth, shadowHeight, shadowMips;
        GetCascadeShadowMapDimensions(cascadeIndex, shadowWidth, shadowHeight, shadowMips);
		
        float shadow = 0;
	
        const float4 lightSpacePos = mul(LB_Lights[aLightIndex].LightViewProjTexture[cascadeIndex], aPixelWorldPosition);
        const float3 convertedPos = lightSpacePos.xyz / lightSpacePos.w;
        const float2 shadowUV = convertedPos.xy;
		
        float2 dU_dXY = float2(ddx_fine(lightSpacePos.x), ddy_fine(lightSpacePos.x));
        float2 dV_dXY = float2(ddx_fine(lightSpacePos.y), ddy_fine(lightSpacePos.y));
        float2 dz_dXY = float2(ddx_fine(lightSpacePos.z), ddy_fine(lightSpacePos.z));
		
        float detJ = dU_dXY.x * dV_dXY.y - dV_dXY.x * dU_dXY.y;
		
        float2 dz_dUV = 0;

        if (detJ != 0)
        {
            dz_dUV.x = dot(float2(dV_dXY.y, -dV_dXY.x), dz_dXY) * rcp(detJ);
            dz_dUV.y = dot(float2(-dU_dXY.y, dU_dXY.x), dz_dXY) * rcp(detJ);
        }
		
        float2 distanceToTexelCenterUV = 0.5 - frac(lightSpacePos.xy * shadowWidth);
        distanceToTexelCenterUV *= rcp(float(shadowWidth));

        float slopeBias = min(0, dot(distanceToTexelCenterUV, dz_dUV));
		
        const float p = floor((convertedPos.z + slopeBias) * 65535) / 65535;
			
        const float shadowTexelSize = 1.0f / shadowWidth;
			
        const float3 lightDir = normalize(LB_Lights[aLightIndex].Position - aPixelWorldPosition.xyz);
        const float shadowBias = clamp(factor * (1.0f - dot(aPixelNormal, lightDir)), minBias, maxBias);
        for (int i = 0; i < 16; ++i)
        {
            const float2 offset = poissonDisk16[i] * shadowTexelSize;
            shadow += SampleCascadeShadowMap(cascadeIndex, shadowUV + offset, p - shadowBias);
        }
        shadow /= 16;
	
        return shadow;
    }
    else
    {
        return 1;
    }
}

float GetPointLightShadow(uint aLightIndex, float4 aPixelWorldPosition, float3 aPixelNormal, inout uint aPointLightShadowMapIndex)
{
    if (LB_Lights[aLightIndex].CastShadows)
    {
        const float near = 1.0f;
        const float far = LB_Lights[aLightIndex].Radius;
	
        const float3 toPixel = aPixelWorldPosition.xyz - LB_Lights[aLightIndex].Position;
        const float3 toLightDir = normalize(toPixel);
	
        const float3 toPixelAbs = abs(toPixel);
        const float Z = max(toPixelAbs.x, max(toPixelAbs.y, toPixelAbs.z));
	
        const float fRange = far / (far - near);
        const float v = -fRange * near;
	
        float D = (fRange * Z + v) / Z;
        const float shadowBias = clamp(factor * (1.0f - dot(aPixelNormal, -toLightDir)), minBias, maxBias);
        D += shadowBias;
	
        const float shadow = SamplePointLightShadow(aPointLightShadowMapIndex, toLightDir, D);
        ++aPointLightShadowMapIndex;
	
        return shadow;
    }
    else
    {
        return 1;
    }
}

float GetSpotLightShadow(uint aLightIndex, float4 aPixelWorldPosition, float3 aPixelNormal, inout uint aSpotLightShadowMapIndex)
{
    if (LB_Lights[aLightIndex].CastShadows)
    {
        float shadow = 0;
	
        const float4 lightSpacePos = mul(LB_Lights[aLightIndex].LightViewProjTexture[0], aPixelWorldPosition);
        const float3 convertedPos = lightSpacePos.xyz / lightSpacePos.w;
        const float2 shadowUV = convertedPos.xy;
        const float p = convertedPos.z;
			
        const uint shadowWidth = GetSpotLightShadowMapWidth(aSpotLightShadowMapIndex, 0);
        const float shadowTexelSize = 1.0f / shadowWidth;
			
        const float3 lightDir = normalize(LB_Lights[aLightIndex].Position - aPixelWorldPosition.xyz);
        const float shadowBias = clamp(factor * (1.0f - dot(aPixelNormal, lightDir)), minBias, maxBias);
        for (int i = 0; i < 16; ++i)
        {
            const float2 offset = poissonDisk16[aLightIndex] * shadowTexelSize;
            shadow += SampleSpotLightShadow(aSpotLightShadowMapIndex, shadowUV + offset, p - shadowBias);
        }
        shadow /= 16;
	
        ++aSpotLightShadowMapIndex;
	
        return shadow;
    }
    else
    {
        return 1;
    }
}

#endif