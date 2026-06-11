#include "Common.hlsli"
#include "Material.hlsli"
#include "MaterialParameters.hlsli"

float4 main(VStoPS aPixel) : SV_TARGET
{
	MaterialPixelParameters parameters;
	parameters.PixelColor = aPixel.Color;
	Material_Pixel(parameters);

	return parameters.PixelColor;
}