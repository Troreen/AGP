#include "BloomHelper.hlsli"
#include "../Internal/Common.hlsli"
#include "../Internal/Samplers.hlsli"

float4 main(FullTextureVertex aInput) : SV_TARGET
{
    float x = BB_FilterRadius;
    float y = BB_FilterRadius;
    
    float3 a = Resource.Sample(LinearClamp, float2(aInput.UV.x - x, aInput.UV.y + y)).rgb;
    float3 b = Resource.Sample(LinearClamp, float2(aInput.UV.x, aInput.UV.y + y)).rgb;
    float3 c = Resource.Sample(LinearClamp, float2(aInput.UV.x + x, aInput.UV.y + y)).rgb;

    float3 d = Resource.Sample(LinearClamp, float2(aInput.UV.x - x, aInput.UV.y)).rgb;
    float3 e = Resource.Sample(LinearClamp, float2(aInput.UV.x, aInput.UV.y)).rgb;
    float3 f = Resource.Sample(LinearClamp, float2(aInput.UV.x + x, aInput.UV.y)).rgb;

    float3 g = Resource.Sample(LinearClamp, float2(aInput.UV.x - x, aInput.UV.y - y)).rgb;
    float3 h = Resource.Sample(LinearClamp, float2(aInput.UV.x, aInput.UV.y - y)).rgb;
    float3 i = Resource.Sample(LinearClamp, float2(aInput.UV.x + x, aInput.UV.y - y)).rgb;
    
    float3 upsample = e * 4.0;
    upsample += (b + d + f + h) * 2.0;
    upsample += (a + c + g + i);
    upsample *= 1.0 / 16.0;
    
    return float4(upsample, 0);
}
