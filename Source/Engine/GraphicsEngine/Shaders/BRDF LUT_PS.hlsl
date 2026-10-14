#include "LUTFunctions.hlsli"
#include "Material/Common.hlsli"

float2 main(FullTextureVertex aInput) : SV_TARGET
{
    float2 integratedBRDF = IntegrateBRDF(aInput.UV.x, aInput.UV.y);
	return integratedBRDF;
}