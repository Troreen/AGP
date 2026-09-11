#include "../Material/Samplers.hlsli"

Texture2D GBufferAlbedo : register(t0);
Texture2D GBufferNormal : register(t1);
Texture2D GBufferWorldPosition : register(t4);

struct FullTextureVertex
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD0;
};

float4 main(FullTextureVertex aPixel) : SV_TARGET
{
    if (GBufferAlbedo.Sample(TrilinearClamp, aPixel.UV).a == 0.0f)
        return 1.0f;

    const float3 position = GBufferWorldPosition.Sample(TrilinearClamp, aPixel.UV).xyz;
    const float3 normal = normalize(GBufferNormal.Sample(TrilinearClamp, aPixel.UV).xyz);
    const float2 texel = float2(abs(ddx(aPixel.UV.x)), abs(ddy(aPixel.UV.y)));
    const float2 offsets[8] = {
        float2(1, 0), float2(-1, 0), float2(0, 1), float2(0, -1),
        float2(1, 1), float2(-1, 1), float2(1, -1), float2(-1, -1) };

    float occlusion = 0.0f;
    [unroll]
    for (uint index = 0; index < 8; ++index)
    {
        const float3 samplePosition = GBufferWorldPosition.Sample(TrilinearClamp, aPixel.UV + offsets[index] * texel * 4.0f).xyz;
        const float3 toSample = samplePosition - position;
        const float distanceToSample = length(toSample);
        if (distanceToSample > 0.001f)
        {
            const float nearby = saturate(1.0f - distanceToSample / 100.0f);
            occlusion += nearby * saturate(dot(normal, toSample / distanceToSample));
        }
    }

    const float ao = saturate(1.0f - occlusion / 8.0f);
    return float4(ao, ao, ao, 1.0f);
}
