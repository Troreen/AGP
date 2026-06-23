static const float PI = 3.14159265f;
static const uint LIGHT_TYPE_DIRECTIONAL = 0;
static const uint LIGHT_TYPE_POINT = 1;
static const uint LIGHT_TYPE_SPOT = 2;
static const uint MAX_LIGHTS = 8;
static const uint SHADOW_SETTINGS_DEPTH_BIAS = 0;

struct Light
{
    float3 Color;
    float Intensity;

    float3 Position;
    uint Type;

    float3 Direction;
    float InnerCone;

    float OuterCone;
    float Radius;
    uint ShadowMapIndex;
    uint NumCascades;

    row_major float4x4 LightViewProjTexture[4];
    float4 CascadeSplits;
    float4 ShadowSettings;
};

cbuffer LightBuffer : register(b4)
{
    Light LB_Lights[MAX_LIGHTS];
    uint LB_NumActiveLights;
    float3 __LB_padding;
}

float GetShadowDepthBias(Light aLight)
{
    return aLight.ShadowSettings[SHADOW_SETTINGS_DEPTH_BIAS];
}

float3 LinearToGamma(float3 aColor)
{
    return pow(abs(aColor), 1.0f / 2.2f);
}

float3 SampleWorldNormal(float2 aUV, float3 aNormal, float3 aTangent, float3 aBinormal)
{
    float2 normalXY = NormalTexture.Sample(TrilinearWrap, aUV).rg;
    normalXY = normalXY * 2.0f - 1.0f;

    const float normalZ = sqrt(1.0f - saturate(dot(normalXY, normalXY)));
    float3 tangentNormal = normalize(float3(normalXY, normalZ));

    const float3x3 TBN = float3x3(
        normalize(aTangent),
        normalize(aBinormal),
        normalize(aNormal)
    );

    return normalize(mul(tangentNormal, TBN));
}

float3 Diffuse_BRDF(float3 aDiffuseColor)
{
    return aDiffuseColor / PI;
}

float NormalDistributionFunction_GGX(float aRoughness, float3 aNormal, float3 aHalfVector)
{
    const float alpha = aRoughness * aRoughness;
    const float alpha2 = alpha * alpha;
    const float NdotH = saturate(dot(aNormal, aHalfVector));
    const float NdotH2 = NdotH * NdotH;
    float denominator = NdotH2 * (alpha2 - 1.0f) + 1.0f;
    denominator = PI * denominator * denominator;
    return alpha2 / max(denominator, 0.00001f);
}

float3 Fresnel_SphericalGaussianSchlick(float3 aSpecularColor, float3 aViewDir, float3 aHalfVector)
{
    const float VdotH = saturate(dot(aViewDir, aHalfVector));
    const float power = (-5.55473f * VdotH - 6.98316f) * VdotH;
    return aSpecularColor + (1.0f - aSpecularColor) * exp2(power);
}

float GeometricAttenuation_Schlick_GGX_G1(float aRoughness, float aNdotX)
{
    const float k = ((aRoughness + 1.0f) * (aRoughness + 1.0f)) / 8.0f;
    return aNdotX / max(aNdotX * (1.0f - k) + k, 0.00001f);
}

float GeometricAttenuation_Schlick_GGX(float aRoughness, float3 aNormal, float3 aLightDir, float3 aViewDir)
{
    const float NdotL = saturate(dot(aNormal, aLightDir));
    const float NdotV = saturate(dot(aNormal, aViewDir));
    return GeometricAttenuation_Schlick_GGX_G1(aRoughness, NdotL)
        * GeometricAttenuation_Schlick_GGX_G1(aRoughness, NdotV);
}

float3 Specular_BRDF(
    float aRoughness,
    float3 aNormal,
    float3 aHalfVector,
    float3 aViewDir,
    float3 aLightDir,
    float3 aSpecularColor)
{
    const float D = NormalDistributionFunction_GGX(aRoughness, aNormal, aHalfVector);
    const float3 F = Fresnel_SphericalGaussianSchlick(aSpecularColor, aViewDir, aHalfVector);
    const float G = GeometricAttenuation_Schlick_GGX(aRoughness, aNormal, aLightDir, aViewDir);

    const float NdotL = saturate(dot(aNormal, aLightDir));
    const float NdotV = saturate(dot(aNormal, aViewDir));
    const float denominator = max(4.0f * NdotL * NdotV, 0.00001f);

    return (D * F * G) / denominator;
}

float3 CalculateDirectPBL(
    float3 aLightColor,
    float aIlluminance,
    float3 aLightDir,
    float3 aDiffuseColor,
    float3 aSpecularColor,
    float aRoughness,
    float3 aNormal,
    float3 aViewDir)
{
    const float NdotL = saturate(dot(aNormal, aLightDir));
    float3 halfVector = aLightDir + aViewDir;
    halfVector = dot(halfVector, halfVector) > 0.00001f ? normalize(halfVector) : aNormal;
    const float3 diffuse = Diffuse_BRDF(aDiffuseColor);
    const float3 specular = Specular_BRDF(aRoughness, aNormal, halfVector, aViewDir, aLightDir, aSpecularColor);
    return (diffuse + specular) * aLightColor * aIlluminance * NdotL;
}

int GetNumMips(TextureCube aCubeMap)
{
    int width = 0;
    int height = 0;
    int mipCount = 0;
    aCubeMap.GetDimensions(0, width, height, mipCount);
    return mipCount;
}

float3 CalculateDiffuseIBL(float3 aPixelNormal, TextureCube aEnvCube)
{
    const int numMips = max(GetNumMips(aEnvCube) - 1, 0);
    return aEnvCube.SampleLevel(TrilinearWrap, aPixelNormal, numMips).rgb;
}

float3 CalculateSpecularIBL(
    float3 aSpecularColor,
    float3 aPixelNormal,
    float3 aViewDir,
    float aRoughness,
    TextureCube aEnvCube)
{
    const int numMips = max(GetNumMips(aEnvCube) - 1, 0);
    const float3 reflection = reflect(-aViewDir, aPixelNormal);
    const float3 envColor = aEnvCube.SampleLevel(TrilinearWrap, reflection, aRoughness * numMips).rgb;
    const float NdotV = saturate(dot(aPixelNormal, aViewDir));
    const float2 brdfLUT = BRDF_LUT_Texture.Sample(LUTSampler, float2(NdotV, aRoughness)).rg;
    return envColor * (aSpecularColor * brdfLUT.x + brdfLUT.y);
}

float3 CalculateAmbientIBL(
    float3 aDiffuseColor,
    float3 aSpecularColor,
    float aRoughness,
    float3 aNormal,
    float3 aViewDir,
    float anAmbientOcclusion)
{
    const float3 diffuseIBL = CalculateDiffuseIBL(aNormal, EnvCubeTexture);
    const float3 specularIBL = CalculateSpecularIBL(aSpecularColor, aNormal, aViewDir, aRoughness, EnvCubeTexture);
    return (aDiffuseColor * diffuseIBL + specularIBL) * anAmbientOcclusion;
}

float3 CalculateDirectionalLight(
    Light aLight,
    float3 aDiffuseColor,
    float3 aSpecularColor,
    float aRoughness,
    float3 aNormal,
    float3 aViewDir)
{
    const float3 lightDirection = normalize(-aLight.Direction);
    return CalculateDirectPBL(
        aLight.Color,
        aLight.Intensity,
        lightDirection,
        aDiffuseColor,
        aSpecularColor,
        aRoughness,
        aNormal,
        aViewDir);
}

float3 CalculatePointLight(
    Light aLight,
    float3 aDiffuseColor,
    float3 aSpecularColor,
    float aRoughness,
    float3 aNormal,
    float3 aWorldPosition,
    float3 aViewDir)
{
    const float3 toLight = aLight.Position - aWorldPosition;
    const float distanceToLight = max(length(toLight), 0.01f);
    const float3 lightDirection = toLight / distanceToLight;

    float rangeAttenuation = saturate(1.0f - (distanceToLight * distanceToLight) / (aLight.Radius * aLight.Radius));
    rangeAttenuation *= rangeAttenuation;

    const float illuminance = (aLight.Intensity / (distanceToLight * distanceToLight)) * rangeAttenuation;
    return CalculateDirectPBL(
        aLight.Color,
        illuminance,
        lightDirection,
        aDiffuseColor,
        aSpecularColor,
        aRoughness,
        aNormal,
        aViewDir);
}

float3 CalculateSpotLight(
    Light aLight,
    float3 aDiffuseColor,
    float3 aSpecularColor,
    float aRoughness,
    float3 aNormal,
    float3 aWorldPosition,
    float3 aViewDir)
{
    const float3 toLight = aLight.Position - aWorldPosition;
    const float distanceToLight = max(length(toLight), 0.01f);
    const float3 lightDirection = toLight / distanceToLight;

    float rangeAttenuation = saturate(1.0f - (distanceToLight * distanceToLight) / (aLight.Radius * aLight.Radius));
    rangeAttenuation *= rangeAttenuation;

    const float spotFactor = saturate(dot(-lightDirection, normalize(aLight.Direction)));
    const float spotAttenuation = smoothstep(cos(aLight.OuterCone), cos(aLight.InnerCone), spotFactor);
    const float illuminance = (aLight.Intensity / (distanceToLight * distanceToLight)) * rangeAttenuation * spotAttenuation;

    return CalculateDirectPBL(
        aLight.Color,
        illuminance,
        lightDirection,
        aDiffuseColor,
        aSpecularColor,
        aRoughness,
        aNormal,
        aViewDir);
}

bool IsValidShadowCoord(float3 aShadowCoord)
{
    return aShadowCoord.x >= 0.0f && aShadowCoord.x <= 1.0f
        && aShadowCoord.y >= 0.0f && aShadowCoord.y <= 1.0f
        && aShadowCoord.z >= 0.0f && aShadowCoord.z <= 1.0f;
}

float SampleDirectionalShadowMap(uint aCascadeIndex, float2 aShadowUV, float aShadowDepth)
{
    float shadow = 1.0f;
    switch (aCascadeIndex)
    {
        case 0:
            shadow = DirectionalShadowMaps[0].SampleCmpLevelZero(ShadowCmpSampler, aShadowUV, aShadowDepth);
            break;
        case 1:
            shadow = DirectionalShadowMaps[1].SampleCmpLevelZero(ShadowCmpSampler, aShadowUV, aShadowDepth);
            break;
        case 2:
            shadow = DirectionalShadowMaps[2].SampleCmpLevelZero(ShadowCmpSampler, aShadowUV, aShadowDepth);
            break;
        case 3:
            shadow = DirectionalShadowMaps[3].SampleCmpLevelZero(ShadowCmpSampler, aShadowUV, aShadowDepth);
            break;
    }
    return shadow;
}

float SampleSpotShadowMap(uint aShadowIndex, float2 aShadowUV, float aShadowDepth)
{
    float shadow = 1.0f;
    switch (aShadowIndex)
    {
        case 0:
            shadow = SpotLightShadowMaps[0].SampleCmpLevelZero(ShadowCmpSampler, aShadowUV, aShadowDepth);
            break;
        case 1:
            shadow = SpotLightShadowMaps[1].SampleCmpLevelZero(ShadowCmpSampler, aShadowUV, aShadowDepth);
            break;
        case 2:
            shadow = SpotLightShadowMaps[2].SampleCmpLevelZero(ShadowCmpSampler, aShadowUV, aShadowDepth);
            break;
        case 3:
            shadow = SpotLightShadowMaps[3].SampleCmpLevelZero(ShadowCmpSampler, aShadowUV, aShadowDepth);
            break;
    }
    return shadow;
}

float SamplePointShadowMap(uint aShadowIndex, float3 aDirection, float aShadowDepth)
{
    float shadow = 1.0f;
    switch (aShadowIndex)
    {
        case 0:
            shadow = PointLightShadowMaps[0].SampleCmpLevelZero(ShadowCmpSampler, aDirection, aShadowDepth);
            break;
        case 1:
            shadow = PointLightShadowMaps[1].SampleCmpLevelZero(ShadowCmpSampler, aDirection, aShadowDepth);
            break;
        case 2:
            shadow = PointLightShadowMaps[2].SampleCmpLevelZero(ShadowCmpSampler, aDirection, aShadowDepth);
            break;
        case 3:
            shadow = PointLightShadowMaps[3].SampleCmpLevelZero(ShadowCmpSampler, aDirection, aShadowDepth);
            break;
    }
    return shadow;
}

float CalculateProjectedShadow(Light aLight, uint aMatrixIndex, float3 aWorldPosition, bool aUseDirectionalMaps)
{
    float shadow = 1.0f;
    const float4 shadowPosition = mul(float4(aWorldPosition, 1.0f), aLight.LightViewProjTexture[aMatrixIndex]);
    const float3 shadowCoord = shadowPosition.xyz / shadowPosition.w;
    if (IsValidShadowCoord(shadowCoord))
    {
        const float shadowDepth = saturate(shadowCoord.z - GetShadowDepthBias(aLight));
        if (aUseDirectionalMaps)
        {
            shadow = SampleDirectionalShadowMap(aMatrixIndex, shadowCoord.xy, shadowDepth);
        }
        else
        {
            shadow = SampleSpotShadowMap(aLight.ShadowMapIndex, shadowCoord.xy, shadowDepth);
        }
    }

    return shadow;
}

uint GetDirectionalCascadeIndex(Light aLight, float aViewDepth);

float CalculateDirectionalShadow(Light aLight, float3 aWorldPosition)
{
    float shadow = 1.0f;
    const float viewDepth = mul(float4(aWorldPosition, 1.0f), FB_View).z;
    const uint cascadeIndex = GetDirectionalCascadeIndex(aLight, viewDepth);

    if (aLight.NumCascades > 0)
    {
        shadow = CalculateProjectedShadow(aLight, cascadeIndex, aWorldPosition, true);
    }

    return shadow;
}

float CalculateSpotShadow(Light aLight, float3 aWorldPosition)
{
    float shadow = 1.0f;
    if (aLight.NumCascades > 0)
    {
        shadow = CalculateProjectedShadow(aLight, 0, aWorldPosition, false);
    }

    return shadow;
}

float CalculatePointShadow(Light aLight, float3 aWorldPosition)
{
    float shadow = 1.0f;
    const float3 toPixel = aWorldPosition - aLight.Position;
    const float distanceToPixel = length(toPixel);
    if (aLight.NumCascades > 0 && distanceToPixel > 0.001f && distanceToPixel <= aLight.Radius)
    {
        const float3 absToPixel = abs(toPixel);
        const float z = max(absToPixel.x, max(absToPixel.y, absToPixel.z));
        const float nearPlane = 1.0f;
        const float farPlane = aLight.Radius;
        const float range = farPlane / (farPlane - nearPlane);
        const float depth = saturate(((range * z) - (range * nearPlane)) / z - GetShadowDepthBias(aLight));
        shadow = SamplePointShadowMap(aLight.ShadowMapIndex, normalize(toPixel), depth);
    }

    return shadow;
}

uint GetDirectionalCascadeIndex(Light aLight, float aViewDepth)
{
    uint cascadeIndex = 0;
    if (aViewDepth > aLight.CascadeSplits.z)
    {
        cascadeIndex = 3;
    }
    else if (aViewDepth > aLight.CascadeSplits.y)
    {
        cascadeIndex = 2;
    }
    else if (aViewDepth > aLight.CascadeSplits.x)
    {
        cascadeIndex = 1;
    }
    return cascadeIndex;
}

float3 CalculateLighting(
    float3 aDiffuseColor,
    float3 aSpecularColor,
    float aRoughness,
    float3 aNormal,
    float3 aWorldPosition,
    float3 aViewDir)
{
    float3 radiance = 0.0f;
    const uint numLights = min(LB_NumActiveLights, MAX_LIGHTS);

    for (uint lightIndex = 0; lightIndex < numLights; ++lightIndex)
    {
        const Light light = LB_Lights[lightIndex];
        if (light.Type == LIGHT_TYPE_DIRECTIONAL)
        {
            radiance += CalculateDirectionalLight(light, aDiffuseColor, aSpecularColor, aRoughness, aNormal, aViewDir)
                * CalculateDirectionalShadow(light, aWorldPosition);
        }
        else if (light.Type == LIGHT_TYPE_POINT)
        {
            radiance += CalculatePointLight(light, aDiffuseColor, aSpecularColor, aRoughness, aNormal, aWorldPosition, aViewDir)
                * CalculatePointShadow(light, aWorldPosition);
        }
        else if (light.Type == LIGHT_TYPE_SPOT)
        {
            radiance += CalculateSpotLight(light, aDiffuseColor, aSpecularColor, aRoughness, aNormal, aWorldPosition, aViewDir)
                * CalculateSpotShadow(light, aWorldPosition);
        }
    }

    return radiance;
}
