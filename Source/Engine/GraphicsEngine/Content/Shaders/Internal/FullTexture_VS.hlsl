struct FullTextureVertex
{
	float4 Position : SV_POSITION;
	float2 UV : TEXCOORD0;
};

FullTextureVertex main(uint vertexID : SV_VertexID)
{
	const float4 positions[4] =
	{
		float4(-1.0f, -1.0f, 0.0f, 1.0f),
		float4(-1.0f,  1.0f, 0.0f, 1.0f),
		float4( 1.0f, -1.0f, 0.0f, 1.0f),
		float4( 1.0f,  1.0f, 0.0f, 1.0f)
	};

	const float2 uvs[4] =
	{
		float2(0.0f, 1.0f),
		float2(0.0f, 0.0f),
		float2(1.0f, 1.0f),
		float2(1.0f, 0.0f)
	};

	FullTextureVertex output;
	output.Position = positions[vertexID];
	output.UV = uvs[vertexID];
	return output;
}
