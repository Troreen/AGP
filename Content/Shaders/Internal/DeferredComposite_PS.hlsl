#include "../Material/Samplers.hlsli"

Texture2D DeferredLighting : register(t0);

struct FullTextureVertex
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD0;
};

float4 main(FullTextureVertex aPixel) : SV_TARGET
{
    const float4 lighting = DeferredLighting.Sample(TrilinearWrap, aPixel.UV);
    return float4(pow(abs(lighting.rgb), 1.0f / 2.2f), lighting.a);
}
