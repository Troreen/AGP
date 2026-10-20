#include "MaterialParameters.hlsli"

cbuffer MaterialBuffer : register(b4)
{
    float4 MB_DiffuseColor = float4(1.0, 1.0, 1.0, 1);
    float MB_Roughness = 0;
    float MB_Metalness = 0;
    float MB_Tiling = 1;
    float __MB_padding;
}

void Material_Vertex(inout MaterialVertexParameters aParameters)
{
    aParameters.UV0 *= MB_Tiling;
    aParameters.UV1 *= MB_Tiling;
}

void Material_Pixel(inout MaterialPixelParameters aParameters)
{
    aParameters.PixelColor *= MB_DiffuseColor;
    aParameters.SurfaceValues.g += MB_Roughness;
    aParameters.SurfaceValues.b += MB_Metalness;

    float3 surfaceNormal = cross(ddx(aParameters.WorldPosition.xyz), ddy(aParameters.WorldPosition.xyz));
    surfaceNormal *= rsqrt(max(dot(surfaceNormal, surfaceNormal), 1.0e-12f));

    // Screen-space derivative winding can change with the view; retain the mesh's outward direction.
    if (dot(surfaceNormal, aParameters.Normal) < 0.0f)
    {
        surfaceNormal = -surfaceNormal;
    }

    const float snowCoverage = smoothstep(0.25f, 0.75f, surfaceNormal.y);
    aParameters.PixelColor.rgb = lerp(aParameters.PixelColor.rgb, float3(1.0f, 1.0f, 1.0f), snowCoverage);
}
