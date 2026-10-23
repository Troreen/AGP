#include "Common.hlsli"
#include "Lighting.hlsli"
#include "Material.hlsli"
#include "MaterialParameters.hlsli"
#include "Tonemap.hlsli"

Texture2D AlbedoTexture : register(t0);

float4 main(VStoPS aPixel) : SV_TARGET
{
    float4 sampleAlbedo = AlbedoTexture.Sample(TrilinearWrap, aPixel.UV0);
    
    if (sampleAlbedo.a < 0.05f)
        discard;
	
    MaterialPixelParameters parameters;
    parameters.PixelColor = aPixel.Color * sampleAlbedo;
    parameters.VertexColor = aPixel.Color;
    parameters.SurfaceValues = float4(0, 0, 0, 0);
    parameters.WorldPosition = aPixel.WorldPosition;
    parameters.UV0 = aPixel.UV0;
    parameters.UV1 = aPixel.UV1;
    parameters.ScreenPosition = aPixel.Position.xy;
    parameters.Normal = aPixel.Normal;
    parameters.VertexNormal = aPixel.Normal;
    parameters.Tangent = aPixel.Tangent;
    parameters.BiNormal = aPixel.BiNormal;
    parameters.Emission = float4(0, 0, 0, 0);
    
    Material_Pixel(parameters);
    
    if (parameters.PixelColor.a < 0.05f)
        discard;
	
    return parameters.PixelColor;
}
