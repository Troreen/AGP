static const float PI = 3.14159265f;

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
