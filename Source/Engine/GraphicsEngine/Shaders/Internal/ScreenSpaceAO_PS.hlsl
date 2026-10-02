#include "../Material/Common.hlsli"
#include "../Material/Samplers.hlsli"

static const uint SSAO_KernelSize = 32;
static const float4 SSAO_Kernel[32] =
{
    float4(0.0543f, 0.0712f, 0.0385f, 0.0f),
    float4(-0.0621f, -0.0415f, 0.0682f, 0.0f),
    float4(0.0182f, -0.0891f, 0.0421f, 0.0f),
    float4(-0.0751f, 0.0612f, 0.0453f, 0.0f),
    float4(0.0882f, -0.0392f, 0.0615f, 0.0f),
    float4(-0.0312f, 0.0981f, 0.0624f, 0.0f),
    float4(0.1041f, 0.0482f, 0.0512f, 0.0f),
    float4(-0.0982f, 0.0781f, 0.0694f, 0.0f),
    float4(-0.0285f, -0.1392f, 0.0871f, 0.0f),
    float4(0.1381f, 0.0862f, 0.0742f, 0.0f),
    float4(-0.1612f, 0.0581f, 0.0823f, 0.0f),
    float4(0.0792f, -0.1541f, 0.1312f, 0.0f),
    float4(0.1712f, -0.1251f, 0.1142f, 0.0f),
    float4(-0.1152f, -0.1852f, 0.1451f, 0.0f),
    float4(0.2251f, 0.0792f, 0.1392f, 0.0f),
    float4(-0.2112f, 0.1481f, 0.1782f, 0.0f),
    float4(0.0481f, -0.2741f, 0.1852f, 0.0f),
    float4(-0.2852f, -0.1582f, 0.2081f, 0.0f),
    float4(0.2881f, 0.2012f, 0.2241f, 0.0f),
    float4(-0.1792f, 0.3281f, 0.2352f, 0.0f),
    float4(0.3621f, -0.2182f, 0.2512f, 0.0f),
    float4(-0.4152f, -0.1761f, 0.2852f, 0.0f),
    float4(0.2112f, 0.4321f, 0.3182f, 0.0f),
    float4(-0.3882f, 0.3551f, 0.3582f, 0.0f),
    float4(0.4851f, -0.2882f, 0.3821f, 0.0f),
    float4(-0.2852f, -0.5281f, 0.4082f, 0.0f),
    float4(0.5521f, 0.3082f, 0.4381f, 0.0f),
    float4(-0.5182f, 0.4221f, 0.4952f, 0.0f),
    float4(0.3751f, -0.6182f, 0.5182f, 0.0f),
    float4(-0.6751f, -0.3281f, 0.5682f, 0.0f),
    float4(0.7082f, 0.4221f, 0.5952f, 0.0f),
    float4(-0.4751f, 0.7281f, 0.8182f, 0.0f)
};

Texture2D GBufferWorldNormalDepth : register(t1);

struct FullTextureVertex
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD0;
};

float3 GetViewSpacePosition(float2 aUV)
{
    float4 normalDepthSample = GBufferWorldNormalDepth.Sample(TrilinearClamp, aUV);
    float z = normalDepthSample.a;
    float x = ((2 * aUV.x - 1) * z) / FB_Projection._11;
    float y = ((1 - aUV.y * 2) * z) / FB_Projection._22;
    return float3(x, y, z);
}

float3 GetViewSpaceNormal(float2 aUV)
{
    float4 normalDepthSample = GBufferWorldNormalDepth.Sample(TrilinearClamp, aUV);
    float3 worldNormal = normalDepthSample.xyz;
    float3x3 viewMatrixRotation = (float3x3) FB_View;
    float3 viewNormal = mul(worldNormal, viewMatrixRotation);
    return normalize(viewNormal);
}

float4 main(FullTextureVertex aInput) : SV_TARGET
{
    const float3 N = GetViewSpaceNormal(aInput.UV);
    const float3 r = float3(0.25f, 0.5f, 0.0f);
    const float3 T = normalize(r - N * dot(r, N));
    const float3 B = cross(N, T);

    const float3x3 TBN = float3x3(T, B, N);
    
    const float radius = 50.0f;
    const float bias = 0.25f;
    float occlusion = 0.0f;
    
    float3 pos = GetViewSpacePosition(aInput.UV);
    
    for (uint i = 0; i < SSAO_KernelSize; ++i)
    {
        float3 kernelPos = mul(TBN, SSAO_Kernel[i].xyz);
        kernelPos = pos.xyz + kernelPos * radius;
        
        float4 offset = float4(kernelPos, 1.0f);
        offset = mul(FB_Projection, offset);
        offset.xyz /= offset.w;
        
        float2 sampleUV = float2(offset.x, -offset.y) * 0.5f + 0.5f;
        const float stepDepth = GetViewSpacePosition(sampleUV).z;
        
        const float range = smoothstep(0.0f, 1.0f, radius / abs(pos.z - stepDepth));
        occlusion += (stepDepth <= kernelPos.z - bias ? 1.0f : 0.0f) * range;
    }
    
    occlusion = 1.0f - (occlusion / (float) SSAO_KernelSize);
    return occlusion;
}