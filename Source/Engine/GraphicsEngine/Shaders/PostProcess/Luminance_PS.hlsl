#include "BloomHelper.hlsli"
#include "../Material/Common.hlsli"
#include "../Material/Lighting.hlsli"

float4 main(FullTextureVertex aVertex) : SV_TARGET
{
    float3 p = Resource.Sample(TrilinearClamp, aVertex.UV).rgb;
    float luminance = dot(p, float3(0.2126, 0.7152, 0.0722));
    
    if (luminance > BB_LuminanceThreshhold)
    {
        return float4(pow(abs(p), abs(BB_LuminanceExponent)), 1.0f);
    }
	
    return 0;
}