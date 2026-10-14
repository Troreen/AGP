#include "SSAOHelper.hlsli"

float4 main(FullTextureVertex aInput) : SV_TARGET
{
    uint width, height, mips;
    Resource.GetDimensions(0, width, height, mips);
    
    float texelSize = 1.0f / width; // Horizontal
    
    const float2 texOffset = float2(texelSize, 0.0f);
    return SSAOBlur(aInput, texOffset);
}