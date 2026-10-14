#define PI 3.14159265358979323846f

// Functions required for setting up the LUT
// NOTE: These are NOT the same functions used when calculating PBL!
// They are only used for calculating the cubemap.
// All you need to call is IntegrateBRDF.

float2 Hammersley(float i, float numSamples)
{
    uint b = uint(i);

    b = (b << 16u) | (b >> 16u);
    b = ((b & 0x55555555u) << 1u) | ((b & 0xAAAAAAAAu) >> 1u);
    b = ((b & 0x33333333u) << 2u) | ((b & 0xCCCCCCCCu) >> 2u);
    b = ((b & 0x0F0F0F0Fu) << 4u) | ((b & 0xF0F0F0F0u) >> 4u);
    b = ((b & 0x00FF00FFu) << 8u) | ((b & 0xFF00FF00u) >> 8u);

    const float radicalInverseVDC = float(b) * 2.3283064365386963e-10;
    return float2((i / numSamples), radicalInverseVDC);
}

float3 ImportanceSampleGGX(float2 aXi, float3 aNormal, float aRoughness)
{
    const float roughnesSq = aRoughness * aRoughness;

    const float phi = 2.0f * PI * aXi.x;
    const float cosTheta = sqrt((1.0f - aXi.y) / (1.0f + (roughnesSq * roughnesSq - 1) * aXi.y));
    const float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

    float3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    const float3 up = abs(aNormal.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
    const float3 tangent = normalize(cross(up, aNormal));
    const float3 bitangent = cross(aNormal, tangent);

    return normalize(tangent * H.x + bitangent * H.y + aNormal * H.z);
}

// LUT creation function for caculating a BRDF lookup for the second half of the split-sum equation.
// Run this once with float2 result = IntegrateBRDF(uv.x, uv.y); Causes a LUT to be generated.

float GeometricAttenuation_Schlick_GGX_IBL(float aNdotV, float aRoughness)
{
    // Note different k here when calculating G GGX for IBL!
    const float a = aRoughness;
    const float k = (a * a) / 2.0f;

    const float nominator = aNdotV;
    const float denominator = aNdotV * (1.0 - k) + k;

    return nominator / denominator;
}

float GeometricAttenuation_Smith_IBL(float3 aN, float3 aV, float3 aL, float aRoughness)
{
    const float NdotV = saturate(dot(aN, aV));
    const float NdotL = saturate(dot(aN, aL));
    
    const float GGX_NdotV = GeometricAttenuation_Schlick_GGX_IBL(NdotV, aRoughness);
    const float GGX_NdotL = GeometricAttenuation_Schlick_GGX_IBL(NdotL, aRoughness);

    return GGX_NdotL * GGX_NdotV;
}

float2 IntegrateBRDF(float aNdotV, float aRoughness)
{
    float3 V;
    V.x = sqrt(1.0f - aNdotV * aNdotV);
    V.y = 0.0;
    V.z = aNdotV;

    float a = 0;
    float b = 0;

    const float3 N = float3(0, 0, 1);

    const uint NUM_SAMPLES = 1024u;
    for (uint i = 0u; i < NUM_SAMPLES; ++i)
    {
        const float2 xi = Hammersley(i, NUM_SAMPLES);
        const float3 H = ImportanceSampleGGX(xi, N, aRoughness);
        const float3 L = normalize(2.0 * dot(V, H) * H - V);

        const float NdotL = saturate(L.z);
        const float NdotH = saturate(H.z);
        const float VdotH = saturate(dot(V, H));
        const float NdotV = saturate(dot(N, V));

        if(NdotL > 0.0)
        {
            const float G = GeometricAttenuation_Smith_IBL(N, V, L, aRoughness);
            const float G_Vis = (G * VdotH) / (NdotH * NdotV);
            const float Fc = pow(1.0 - VdotH, 5.0);

            a += (1.0 - Fc) * G_Vis;
            b += Fc * G_Vis;
        }
    }

    return float2(a, b) / NUM_SAMPLES;
}