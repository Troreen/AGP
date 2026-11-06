#ifndef TONEMAP_HLSLI
#define TONEMAP_HLSLI

// Pick one of these at a time. Keep in mind that ACES and Lottes need to be
// corrected after output but no the Unreal one!

static const float3x3 ACESInputMat =
{
    { 0.59719, 0.35458, 0.04823 },
    { 0.07600, 0.90834, 0.01566 },
    { 0.02840, 0.13383, 0.83777 }
};

// ODT_SAT => XYZ => D60_2_D65 => sRGB
static const float3x3 ACESOutputMat =
{
    { 1.60475, -0.53108, -0.07367 },
    { -0.10208, 1.10813, -0.00605 },
    { -0.00327, -0.07276, 1.07602 }
};

float3 RRTAndODTFit(float3 v)
{
    float3 a = v * (v + 0.0245786f) - 0.000090537f;
    float3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
    return a / b;
}

float3 Tonemap_ACES(float3 aColor)
{
    aColor = mul(ACESInputMat, aColor);
    aColor = RRTAndODTFit(aColor);
    aColor = mul(ACESOutputMat, aColor);
    aColor = saturate(aColor);

    return aColor;
}

float3 Tonemap_UnrealEngine(float3 aColor)
{
    // Unreal 3, Documentation: "Color Grading"
	// Adapted to be close to Tonemap_ACES, with similar range
	// Gamma 2.2 correction is baked in, don't use with sRGB conversion!
	return aColor / (aColor + 0.155) * 1.019;
}


float3 Tonemap_Lottes(float3 aColor)
{
	// Lottes 2016, "Advanced Techniques and Optimization of HDR Color Pipelines"
	const float a = 1.6;
	const float d = 0.977;
	const float hdrMax = 8.0;
	const float midIn = 0.18;
	const float midOut = 0.267;

	// Can be precomputed
	const float b =
		(-pow(abs(midIn), a) + pow(abs(hdrMax), a) * midOut) /
		((pow(abs(hdrMax), a * d) - pow(abs(midIn), a * d)) * midOut);
	const float c =
		(pow(abs(hdrMax), a * d) * pow(abs(midIn), a) - pow(abs(hdrMax), a) * pow(abs(midIn), a * d) * midOut) /
		((pow(abs(hdrMax), a * d) - pow(abs(midIn), a * d)) * midOut);

	return pow(abs(aColor), a) / (pow(abs(aColor), a * d) * b + c);
}

float3 LinearToGamma(float3 aColor)
{
	return pow(abs(aColor), 1.0 / 2.2);
}

#endif