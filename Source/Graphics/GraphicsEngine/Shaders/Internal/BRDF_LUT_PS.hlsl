static const float PI = 3.14159265f;
static const uint SAMPLE_COUNT = 1024;

struct FullTextureVertex
{
	float4 Position : SV_POSITION;
	float2 UV : TEXCOORD0;
};

float RadicalInverse_VdC(uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10f;
}

float2 Hammersley(uint sampleIndex, uint sampleCount)
{
	return float2(float(sampleIndex) / float(sampleCount), RadicalInverse_VdC(sampleIndex));
}

float3 ImportanceSampleGGX(float2 randomValue, float roughness, float3 normal)
{
	const float alpha = roughness * roughness;
	const float phi = 2.0f * PI * randomValue.x;
	const float cosTheta = sqrt((1.0f - randomValue.y) / (1.0f + (alpha * alpha - 1.0f) * randomValue.y));
	const float sinTheta = sqrt(max(1.0f - cosTheta * cosTheta, 0.0f));

	const float3 halfVectorTangent = float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
	const float3 up = abs(normal.z) < 0.999f ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
	const float3 tangent = normalize(cross(up, normal));
	const float3 bitangent = cross(normal, tangent);

	return normalize(tangent * halfVectorTangent.x + bitangent * halfVectorTangent.y + normal * halfVectorTangent.z);
}

float GeometrySchlickGGX(float NdotX, float roughness)
{
	const float alpha = roughness * roughness;
	const float k = alpha * 0.5f;
	return NdotX / max(NdotX * (1.0f - k) + k, 0.00001f);
}

float GeometrySmith(float3 normal, float3 viewDir, float3 lightDir, float roughness)
{
	const float NdotV = saturate(dot(normal, viewDir));
	const float NdotL = saturate(dot(normal, lightDir));
	return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

float2 IntegrateBRDF(float NdotV, float roughness)
{
	const float3 normal = float3(0.0f, 0.0f, 1.0f);
	float3 viewDir = float3(sqrt(max(1.0f - NdotV * NdotV, 0.0f)), 0.0f, NdotV);

	float scale = 0.0f;
	float bias = 0.0f;

	[loop]
	for (uint sampleIndex = 0; sampleIndex < SAMPLE_COUNT; ++sampleIndex)
	{
		const float2 randomValue = Hammersley(sampleIndex, SAMPLE_COUNT);
		const float3 halfVector = ImportanceSampleGGX(randomValue, roughness, normal);
		const float3 lightDir = normalize(2.0f * dot(viewDir, halfVector) * halfVector - viewDir);

		const float NdotL = saturate(lightDir.z);
		const float NdotH = saturate(halfVector.z);
		const float VdotH = saturate(dot(viewDir, halfVector));

		if (NdotL > 0.0f)
		{
			const float geometry = GeometrySmith(normal, viewDir, lightDir, roughness);
			const float geometryVisibility = (geometry * VdotH) / max(NdotH * NdotV, 0.00001f);
			const float fresnel = pow(1.0f - VdotH, 5.0f);

			scale += (1.0f - fresnel) * geometryVisibility;
			bias += fresnel * geometryVisibility;
		}
	}

	return float2(scale, bias) / float(SAMPLE_COUNT);
}

float2 main(FullTextureVertex input) : SV_TARGET
{
	return IntegrateBRDF(input.UV.x, input.UV.y);
}
