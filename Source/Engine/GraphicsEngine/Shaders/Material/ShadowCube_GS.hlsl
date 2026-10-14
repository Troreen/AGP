#include "Common.hlsli"
#include "Lighting.hlsli"

struct ShadowGSOut
{
	float4 Position : SV_POSITION;
	uint CubeFace : SV_RenderTargetArrayIndex;
};

static const float4x4 cubeViews[6] =
{
	float4x4(
		0, 0, -1, 0,
		0, 1, 0, 0,
		1, 0, 0, 0,
		0, 0, 0, 1
	),
	float4x4(
		0, 0, 1, 0,
		0, 1, 0, 0,
		-1, 0, 0, 0,
		0, 0, 0, 1
	),
	
	float4x4(
		1, 0, 0, 0,
		0, 0, -1, 0,
		0, 1, 0, 0,
		0, 0, 0, 1
	),
	float4x4(
		1, 0, 0, 0,
		0, 0, 1, 0,
		0, -1, 0, 0,
		0, 0, 0, 1
	),
	
	float4x4(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	),
	float4x4(
		-1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, -1, 0,
		0, 0, 0, 1
	)
};

[maxvertexcount(18)]
void main(
	triangle VStoPS input[3],
	inout TriangleStream<ShadowGSOut> output
)
{
	for (int f = 0; f < 6; ++f)
	{
		ShadowGSOut vertex;
		for (int v = 0; v < 3; ++v)
		{
			float3 posRelativeToLight = input[v].WorldPosition.xyz - LB_Lights[0].Position;
			float4 viewSpacePos = mul(cubeViews[f], float4(posRelativeToLight, 1.0f));
			vertex.Position = mul(FB_Projection, viewSpacePos);
			vertex.CubeFace = f;
			output.Append(vertex);
		}
		output.RestartStrip();
	}
}