cbuffer TextOverlayBuffer : register(b5)
{
	float2 ClientSize;
	float2 PixelOrigin;
	float Opacity;
	float DistanceRange;
	float2 AtlasSize;
};

struct VertexInput
{
	float4 Position : POSITION;
	float4 Color : COLOR;
	uint4 BoneIDs : BONEIDS;
	float4 SkinWeights : SKINWEIGHTS;
	float2 UV : UV0;
	float2 UV1 : UV1;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
};

struct VertexOutput
{
	float4 Position : SV_POSITION;
	float4 Color : COLOR;
	float2 UV : TEXCOORD0;
};

VertexOutput main(VertexInput input)
{
	VertexOutput output;
	float2 pixel = PixelOrigin + input.Position.xy;
	output.Position = float4(pixel.x / ClientSize.x * 2.0f - 1.0f, 1.0f - pixel.y / ClientSize.y * 2.0f, 0.0f, 1.0f);
	output.Color = float4(input.Color.rgb, input.Color.a * Opacity);
	output.UV = input.UV;
	return output;
}
