#include "BloomHelper.hlsli"
#include "../Internal/Common.hlsli"
#include "../Internal/Samplers.hlsli"

float4 main(FullTextureVertex aInput) : SV_TARGET
{
    float3 sample = Resource.Sample(LinearClamp, aInput.UV).rgb;
    float luminance = dot(sample, float3(0.2126, 0.7152, 0.0722));
    
    float knee = BB_Threshold * BB_Softness;
    float soft = luminance - (BB_Threshold - knee);
    soft = clamp(soft, 0.0, 2.0 * knee);
    soft = (soft * soft) / (4.0 * knee + 1e-5);
    
    float weight = max(soft, luminance - BB_Threshold);
    weight = max(weight, 0.0) / max(luminance, 1e-5);
    
    return float4(sample * weight, 1);
}
