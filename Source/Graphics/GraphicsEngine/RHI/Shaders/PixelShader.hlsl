#include "Common.hlsli"

float4 main(VStoPS aPixel) : SV_TARGET
{
	return aPixel.Color; 
}