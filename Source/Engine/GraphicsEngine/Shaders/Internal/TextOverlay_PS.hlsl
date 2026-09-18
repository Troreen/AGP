Texture2D FontAtlas : register(t0);
SamplerState FontSampler : register(s0);

cbuffer TextOverlayBuffer : register(b5)
{
	float2 ClientSize;
	float2 PixelOrigin;
	float Opacity;
	float DistanceRange;
	float2 AtlasSize;
};

struct PixelInput
{
	float4 Position : SV_POSITION;
	float4 Color : COLOR;
	float2 UV : TEXCOORD0;
};

float Median(float3 value)
{
	return max(min(value.r, value.g), min(max(value.r, value.g), value.b));
}

float4 main(PixelInput input) : SV_TARGET
{
	float signedDistance = Median(FontAtlas.Sample(FontSampler, input.UV).rgb) - 0.5f;
	float2 unitRange = DistanceRange / AtlasSize;
	float2 screenTexSize = 1.0f / fwidth(input.UV);
	float screenDistance = signedDistance * max(0.5f * dot(unitRange, screenTexSize), 1.0f);
	float coverage = saturate(screenDistance + 0.5f);
	return float4(input.Color.rgb, input.Color.a * coverage);
}
