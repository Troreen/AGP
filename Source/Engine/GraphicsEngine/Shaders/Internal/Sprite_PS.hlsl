#include "../Material/Samplers.hlsli"
#include "../Material/Common.hlsli"
Texture2D SpriteTexture : register(t0);

float4 main(VStoPS aPixel) : SV_TARGET
{
    float4 color = SpriteTexture.Sample(TrilinearWrap, aPixel.UV0);
    if (color.a <= 0)
    {
        discard;
    }
    return color;
}