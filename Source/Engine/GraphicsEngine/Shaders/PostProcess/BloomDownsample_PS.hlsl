#include "BloomHelper.hlsli"
#include "../Internal/Common.hlsli"
#include "../Internal/Samplers.hlsli"

float4 main(FullTextureVertex aInput) : SV_TARGET
{
    uint width, height, mips;
    Resource.GetDimensions(0, width, height, mips);
    
    float x = 1.0f / width;
    float y = 1.0f / height;
    
    float3 a = Resource.Sample(LinearClamp, float2(aInput.UV.x - 2 * x, aInput.UV.y + 2 * y)).rgb;
    float3 b = Resource.Sample(LinearClamp, float2(aInput.UV.x, aInput.UV.y + 2 * y)).rgb;
    float3 c = Resource.Sample(LinearClamp, float2(aInput.UV.x + 2 * x, aInput.UV.y + 2 * y)).rgb;

    float3 d = Resource.Sample(LinearClamp, float2(aInput.UV.x - 2 * x, aInput.UV.y)).rgb;
    float3 e = Resource.Sample(LinearClamp, float2(aInput.UV.x, aInput.UV.y)).rgb;
    float3 f = Resource.Sample(LinearClamp, float2(aInput.UV.x + 2 * x, aInput.UV.y)).rgb;

    float3 g = Resource.Sample(LinearClamp, float2(aInput.UV.x - 2 * x, aInput.UV.y - 2 * y)).rgb;
    float3 h = Resource.Sample(LinearClamp, float2(aInput.UV.x, aInput.UV.y - 2 * y)).rgb;
    float3 i = Resource.Sample(LinearClamp, float2(aInput.UV.x + 2 * x, aInput.UV.y - 2 * y)).rgb;

    float3 j = Resource.Sample(LinearClamp, float2(aInput.UV.x - x, aInput.UV.y + y)).rgb;
    float3 k = Resource.Sample(LinearClamp, float2(aInput.UV.x + x, aInput.UV.y + y)).rgb;
    float3 l = Resource.Sample(LinearClamp, float2(aInput.UV.x - x, aInput.UV.y - y)).rgb;
    float3 m = Resource.Sample(LinearClamp, float2(aInput.UV.x + x, aInput.UV.y - y)).rgb;
    
    float3 downsample = e * 0.125;
    downsample += (a + c + g + i) * 0.03125;
    downsample += (b + d + f + h) * 0.0625;
    downsample += (j + k + l + m) * 0.125;
    
    return float4(downsample, 0);
}