#include "MaterialParameters.hlsli"
#include "Normal.hlsli"
#include "MaterialUtilities.hlsli"

void Material_Vertex(inout MaterialVertexParameters aParameters)
{
}

// Use every helper through the real material entry points so missing inputs or includes fail compilation.
void Material_Pixel(inout MaterialPixelParameters aParameters)
{
    const float3 base = Normal_DecodeRG(float2(0.6f, 0.4f));
    // Keep the reference graph's multiply, subtract, and recover-Z steps visible.
    const float2 detailXY = aParameters.UV0 * 2.0f - 1.0f;
    const float3 detail = normalize(Normal_DeriveZ(detailXY));
    const float3 normalTS = Normal_BlendAngleCorrected(base, detail);
    aParameters.Normal = Normal_TangentToWorld(normalTS, aParameters.VertexNormal,
        aParameters.Tangent, aParameters.BiNormal);
    const float3 view = View_DirectionToCamera(aParameters.WorldPosition.xyz);
    const float rim = Fresnel_Schlick(aParameters.Normal, view, 0.04f, 6.0f);
    const float2 screenUV = UV_ScreenTexture(aParameters.ScreenPosition, float2(512, 512));
    const float sunFacing = saturate(dot(normalize(aParameters.VertexNormal), Sun_DirectionToLight()));
    aParameters.PixelColor.rgb = float3(frac(screenUV), rim * sunFacing);
    aParameters.SurfaceValues.g = rim;
    aParameters.Emission.rgb = rim * sunFacing;
}
