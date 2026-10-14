#include "SSAOHelper.hlsli"

float4 main(FullTextureVertex aInput) : SV_TARGET
{
    uint width, height, mips;
    Resource.GetDimensions(0, width, height, mips);
    
    float texelSize = 1.0f / height; // Vertical
    
    const float2 texOffset = float2(0.0f, texelSize);
    return SSAOBlur(aInput, texOffset);
}