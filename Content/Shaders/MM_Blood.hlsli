#include "MaterialParameters.hlsli"
#include "Lighting.hlsli"

cbuffer MaterialBuffer : register(b4)
{
    float4 MB_DiffuseColor = float4(1.0, 1.0, 1.0, 1);
    float MB_Roughness = 0;
    float MB_Metalness = 0;
    float MB_Tiling = 1;
    float __MB_padding;
}

Texture2D MergedDistortNoiseChannels : register(t4);
Texture2D MergedBloodNoiseChannels : register(t5);

void Material_Vertex(inout MaterialVertexParameters aParameters)
{
    aParameters.UV0 *= MB_Tiling;
    aParameters.UV1 *= MB_Tiling;
}

void Material_Pixel(inout MaterialPixelParameters aParameters)
{
    aParameters.PixelColor *= MB_DiffuseColor;
    aParameters.SurfaceValues.g += MB_Roughness;
    aParameters.SurfaceValues.b += MB_Metalness;

        //DistortionNoise A (This and DistortB will run over the main texture)
        // UV + Time * float2 -> Sample -> Append (2 channels from the texture that you want from it)
    float2 uv_A = aParameters.UV0 + FB_TotalTime * float2(0.05, -0.05);
    float2 distort_A = MergedBloodNoiseChannels.Sample(TrilinearWrap, uv_A).rb;
        //DistortionNoise B
    float2 uv_B = aParameters.UV0 + FB_TotalTime * float2(0.1, -0.1);
    float2 distort_B = MergedBloodNoiseChannels.Sample(TrilinearWrap, uv_B).rg;

        //Add both, then multiply by 0.5
    float2 distortion = (distort_A + distort_B) * 0.5;

        // Tiling & Offset, then we add the distortion before plugging it to MainTexture
        // UV_C is for the main texture
    float2 uv_C = aParameters.UV0 + FB_TotalTime * float2(-0.1, 0.1);
    float2 finalUV = uv_C + distortion;

    float3 main_Texture = MergedDistortNoiseChannels.Sample(TrilinearWrap, finalUV).rgb; //".rgb = Make Vector3"

        //Colorize
    float3 colorA = float3(0.5531, 0.0, 1.0); //Bright Color
    float3 colorB = float3(0.0527, 0.0, 0.1406); //Darker Color
    // Use the animated noise to blend from dark regions to bright regions.
    float tintMask = saturate(main_Texture.r);
    float3 tint = lerp(colorB, colorA, tintMask);

    float3 finalResult = tint * main_Texture; // Multiply color + texture before output

    aParameters.PixelColor.rgb *= finalResult;
}
