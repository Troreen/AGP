#include "DeferredLighting.hlsli"

Texture2D GBufferTangentNormal : register(t5);

cbuffer RenderPassDebugBuffer : register(b5)
{
    uint DebugRenderPass;
    float3 __RenderPassDebugPadding;
}

float4 main(FullTextureVertex aPixel) : SV_TARGET
{
    const float4 albedo = GBufferAlbedo.Sample(TrilinearClamp, aPixel.UV);
    if (albedo.a == 0.0f)
        return 0.0f;

    const float3 material = GBufferMaterial.Sample(TrilinearClamp, aPixel.UV).rgb;
    float3 result = 0.0f;

    // RenderPass values are deliberately stable because the label and shortcut
    // are exposed to users in every build configuration.
    if (DebugRenderPass == 1)          result = pow(saturate(albedo.rgb), 1.0f / 2.2f); // Albedo, sRGB
    else if (DebugRenderPass == 2)     result = material.ggg;                            // Roughness, linear
    else if (DebugRenderPass == 3)     result = material.bbb;                            // Metalness, linear
    else if (DebugRenderPass == 4)     result = material.rrr;                            // Texture AO, linear
    else if (DebugRenderPass == 5)     result = ScreenSpaceAO.Sample(TrilinearClamp, aPixel.UV).rrr;
    else if (DebugRenderPass == 6)     result = GBufferTangentNormal.Sample(TrilinearClamp, aPixel.UV).xyz * 0.5f + 0.5f;
    else if (DebugRenderPass == 7)     result = GBufferNormal.Sample(TrilinearClamp, aPixel.UV).xyz * 0.5f + 0.5f;
    else if (DebugRenderPass == 8)
    {
        float shadow = 1.0f;
        [loop]
        for (uint lightIndex = 0; lightIndex < LB_NumActiveLights; ++lightIndex)
        {
            if (LB_Lights[lightIndex].Type == LIGHT_TYPE_DIRECTIONAL)
            {
                shadow = min(shadow, CalculateDirectionalShadow(LB_Lights[lightIndex], GBufferWorldPosition.Sample(TrilinearClamp, aPixel.UV).xyz));
            }
        }
        result = shadow.xxx;
    }

    return float4(result, 1.0f);
}
