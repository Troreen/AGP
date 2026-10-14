#include "TextOverlayHelper.hlsli"

Texture2D FontAtlas : register(t0);
SamplerState FontSampler : register(s1);

float Median(float3 value)
{
    return max(min(value.r, value.g), min(max(value.r, value.g), value.b));
}

float4 main(VStoPS aInput) : SV_TARGET
{
    float signedDistance = Median(FontAtlas.Sample(FontSampler, aInput.UV).rgb) - 0.5f;
    float2 unitRange = TB_DistanceRange / TB_AtlasSize;
    float2 screenTexSize = 1.0f / fwidth(aInput.UV);
    float screenDistance = signedDistance * max(0.5f * dot(unitRange, screenTexSize), 1.0f);
    float coverage = saturate(screenDistance + 0.5f);
    return float4(aInput.Color.rgb, aInput.Color.a * coverage);
}
