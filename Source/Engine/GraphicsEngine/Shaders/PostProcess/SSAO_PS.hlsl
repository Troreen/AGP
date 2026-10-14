#include "../Material/Common.hlsli"
#include "../Material/Lighting.hlsli"
#include "SSAOHelper.hlsli"

static const uint SSAO_SAMPLE_COUNT = 8;

float4 main(FullTextureVertex aInput) : SV_TARGET
{
    const float3 position = GBufferWorldPosition.Sample(TrilinearClamp, aInput.UV).xyz;
    const float3 normal = normalize(GBufferWorldNormalDepth.Sample(TrilinearClamp, aInput.UV).xyz);
    const float2 texel = float2(abs(ddx(aInput.UV.x)), abs(ddy(aInput.UV.y)));
    const float2 offsets[SSAO_SAMPLE_COUNT] =
    {
        float2(1, 0), float2(-1, 0), float2(0, 1), float2(0, -1),
        float2(1, 1), float2(-1, 1), float2(1, -1), float2(-1, -1)
    };

    float occlusion = 0.0f;
    [unroll]
    for (uint index = 0; index < SSAO_SAMPLE_COUNT; ++index)
    {
        const float3 samplePosition = GBufferWorldPosition.Sample(TrilinearClamp, aInput.UV + offsets[index] * texel * SB_SampleRadiusTexels).xyz;
        const float3 toSample = samplePosition - position;
        const float distanceToSample = length(toSample);
        
        if (distanceToSample > SB_MinDistance)
        {
            const float nearby = saturate(1.0f - distanceToSample / SB_MaxDistance);
            occlusion += nearby * saturate(dot(normal, toSample / distanceToSample));
        }
    }

    const float ao = saturate(1.0f - occlusion / (float) SSAO_SAMPLE_COUNT);
    return float4(ao, ao, ao, 1.0f);
}