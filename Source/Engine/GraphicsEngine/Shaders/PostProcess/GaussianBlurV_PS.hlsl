#include "BloomHelper.hlsli"
#include "../Material/Common.hlsli"
#include "../Material/Lighting.hlsli"

float4 main(FullTextureVertex aInput) : SV_TARGET
{
    static const float GaussianKernel[5] = { 0.06136f, 0.24477f, 0.38774f, 0.24477f, 0.06136f };
    static const uint kernelSize = 5;
    
    uint width, height, mips;
    Resource.GetDimensions(0, width, height, mips);
    
    float texelSize = 1.0f / height; // Vertical
    float step = (((float)kernelSize - 1.0f) / 2.0f) * -1.0f;
    
    float4 result = 0;
    for (uint s = 0; s < kernelSize; ++s)
    {
        float2 uv = aInput.UV + float2(0.0f, texelSize * (step + (float)s)); // Vertical
        float4 color = Resource.Sample(TrilinearClamp, uv);
        result += color * GaussianKernel[s];
    }
    
    return result;
}