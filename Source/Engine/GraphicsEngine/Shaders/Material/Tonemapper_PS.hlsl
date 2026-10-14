#include "Common.hlsli"
#include "Lighting.hlsli"
#include "Tonemap.hlsli"

Texture2D IntermediateBuffyTheVampireSlayer : register(t0);

float4 main(FullTextureVertex input) : SV_TARGET
{
    float4 sampleBuffer = IntermediateBuffyTheVampireSlayer.Sample(TrilinearClamp, input.UV);
    float3 gammaPixel;
    
    switch (FB_RenderPassType)
    {
        case 0:
        {
            switch (FB_TonemapType)
            {
                case 1:
                    gammaPixel = LinearToGamma(Tonemap_ACES(sampleBuffer.rgb));
                    break;
        
                case 2:
                    gammaPixel = Tonemap_UnrealEngine(sampleBuffer.rgb);
                    break;
        
                case 3:
                    gammaPixel = LinearToGamma(Tonemap_Lottes(sampleBuffer.rgb));
                    break;
        
                default:
                    gammaPixel = LinearToGamma(sampleBuffer.rgb);
                    break;
            }
            
            break;
        }
        
        default:
            gammaPixel = sampleBuffer.rgb;
            break;
    }
    
    return float4(gammaPixel, sampleBuffer.a);
}
