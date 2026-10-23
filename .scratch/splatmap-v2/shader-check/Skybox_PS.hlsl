#include "Common.hlsli"

Texture2D<float> SceneDepth : register(t0);
TextureCube EnvironmentTexture : register(t98);
SamplerState TrilinearClamp : register(s3);

float4 main(FullTextureVertex aInput) : SV_TARGET
{
    // Skip pixels where a mesh has already been drawn.
    if (SceneDepth.Load(int3(int2(aInput.Position.xy), 0)) < 1.0f)
    {
        discard;
    }

    const float2 clipPosition = float2(aInput.UV.x * 2.0f - 1.0f, 1.0f - aInput.UV.y * 2.0f);
    const float3 viewDirection = float3(clipPosition.x / FB_Projection[0][0], clipPosition.y / FB_Projection[1][1], 1.0f);
    // Rotate the ray back into world space. Leave out the camera position
    // so moving the camera does not move the skybox.
    const float3 worldDirection = mul(transpose((float3x3)FB_View), viewDirection);
    return float4(EnvironmentTexture.SampleLevel(TrilinearClamp, normalize(worldDirection), 0).rgb, 1.0f);
}
