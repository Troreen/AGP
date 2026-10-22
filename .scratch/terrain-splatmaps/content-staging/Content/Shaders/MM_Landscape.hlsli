#ifndef MM_LANDSCAPE_HLSLI
#define MM_LANDSCAPE_HLSLI

#include "MaterialParameters.hlsli"
#include "Normal.hlsli"

// Pixel entry points already declare these engine samplers. The vertex entry
// point also parses this include, but does not include Lighting.hlsli.
#ifndef LIGHTING_HLSLI
SamplerState TrilinearWrap : register(s2);
SamplerState TrilinearClamp : register(s3);
#endif

cbuffer MaterialBuffer : register(b4)
{
    // Each mask row maps float4(UnrealWorldXY, 0, 1) to one mask coordinate.
    float4 MB_MaskU = float4(0.0001785714286, 0, 0, 0.5);
    float4 MB_MaskV = float4(0, 0.0001785714286, 0, 0.5);
    // Reciprocal repeat size in centimetres, followed by shared surface phase.
    float4 MB_SurfaceMapping = float4(0.00125, 0.00125, 3.5, 3.5);
    // Strength in [0,1], patch size in centimetres; remaining components reserved.
    float4 MB_NoiseControls = float4(0, 200, 0, 0);
    float MB_DEBUG = 0;
};

Texture2D DunesBaseColor : register(t4);
Texture2D DunesORM : register(t5);
Texture2D DunesNormal : register(t6);
Texture2D RoughBaseColor : register(t7);
Texture2D RoughORM : register(t8);
Texture2D RoughNormal : register(t9);
Texture2D CliffBaseColor : register(t10);
Texture2D CliffORM : register(t11);
Texture2D CliffNormal : register(t12);
Texture2D SplatMask : register(t13);
Texture2D BlendNoise : register(t14);

void Material_Vertex(inout MaterialVertexParameters aParameters)
{
}

// Keep fully painted and absent layers intact; noise only reshapes transitions.
float Landscape_NoisyWeight(float aWeight, float aSignedNoise, float aStrength)
{
    return saturate(aWeight + aSignedNoise * aStrength * aWeight * (1.0 - aWeight));
}

// Orient detail with world projection, while retaining the geometric slope.
// Mesh UV tangents are deliberately unused so rotated floaters share the frame.
float3 Landscape_ProjectNormal(float3 aDetailNormal, float3 aGeometricNormal)
{
    const float geometricLengthSquared = dot(aGeometricNormal, aGeometricNormal);
    const float3 normal = geometricLengthSquared > 1.0e-8
        ? aGeometricNormal * rsqrt(geometricLengthSquared) : float3(0, 1, 0);
    float3 tangent = float3(0, 0, 1) - normal * normal.z;
    if (dot(tangent, tangent) < 1.0e-6)
    {
        // Vertical faces parallel to the projection have no unique U direction.
        tangent = float3(1, 0, 0) - normal * normal.x;
    }
    tangent = normalize(tangent);
    const float3 bitangent = cross(normal, tangent);
    const float detailLengthSquared = dot(aDetailNormal, aDetailNormal);
    const float3 detail = detailLengthSquared > 1.0e-8
        ? aDetailNormal * rsqrt(detailLengthSquared) : float3(0, 0, 1);
    return normalize(detail.x * tangent + detail.y * bitangent + detail.z * normal);
}

void Material_Pixel(inout MaterialPixelParameters aParameters)
{
    // AGP world ZX corresponds to Unreal world XY. Mask and surface transforms
    // are independent: the level mask does not repeat with the surface textures.
    const float2 groundPosition = aParameters.WorldPosition.zx;
    const float4 maskPosition = float4(groundPosition, 0, 1);
    const float2 maskUV = float2(dot(MB_MaskU, maskPosition), dot(MB_MaskV, maskPosition));
    const float3 paintedWeights = saturate(SplatMask.Sample(TrilinearClamp, maskUV).rgb);
    const float2 surfaceUV = groundPosition * MB_SurfaceMapping.xy + MB_SurfaceMapping.zw;
    const float2 noiseUV = groundPosition / max(MB_NoiseControls.y, 1.0);
    const float signedNoise = BlendNoise.Sample(TrilinearWrap, noiseUV).r * 2.0 - 1.0;
    const float strength = saturate(MB_NoiseControls.x);
    const float roughWeight = Landscape_NoisyWeight(paintedWeights.g, signedNoise, strength);
    const float cliffWeight = Landscape_NoisyWeight(paintedWeights.b, signedNoise, strength);

    // Filling uncovered contribution with Dunes makes it the continuous base.
    // Independent painted weights are ordered alpha overlays, not normalized RGB.
    const float3 contributions = float3((1.0 - roughWeight) * (1.0 - cliffWeight),
        roughWeight * (1.0 - cliffWeight), cliffWeight);
    const float3 dunesColor = DunesBaseColor.Sample(TrilinearWrap, surfaceUV).rgb;
    const float3 roughColor = RoughBaseColor.Sample(TrilinearWrap, surfaceUV).rgb;
    const float3 cliffColor = CliffBaseColor.Sample(TrilinearWrap, surfaceUV).rgb;
    const float3 dunesSurface = DunesORM.Sample(TrilinearWrap, surfaceUV).rgb;
    const float3 roughSurface = RoughORM.Sample(TrilinearWrap, surfaceUV).rgb;
    const float3 cliffSurface = CliffORM.Sample(TrilinearWrap, surfaceUV).rgb;
    const float3 dunesNormal = Normal_DecodeRG(DunesNormal.Sample(TrilinearWrap, surfaceUV).rg);
    const float3 roughNormal = Normal_DecodeRG(RoughNormal.Sample(TrilinearWrap, surfaceUV).rg);
    const float3 cliffNormal = Normal_DecodeRG(CliffNormal.Sample(TrilinearWrap, surfaceUV).rg);

    aParameters.PixelColor = float4(dunesColor * contributions.x
        + roughColor * contributions.y + cliffColor * contributions.z, 1);
    aParameters.SurfaceValues = float4(dunesSurface * contributions.x
        + roughSurface * contributions.y + cliffSurface * contributions.z, 0);
    const float3 detailNormal = dunesNormal * contributions.x
        + roughNormal * contributions.y + cliffNormal * contributions.z;
    aParameters.Normal = Landscape_ProjectNormal(detailNormal, aParameters.VertexNormal);
    aParameters.Emission = 0;

    if (MB_DEBUG > 0.5 && MB_DEBUG < 1.5)
    {
        aParameters.PixelColor.rgb = contributions;
    }
    else if (MB_DEBUG >= 1.5 && MB_DEBUG < 2.5)
    {
        aParameters.PixelColor.rgb = paintedWeights;
    }
    else if (MB_DEBUG >= 2.5 && MB_DEBUG < 3.5)
    {
        aParameters.PixelColor.rgb = aParameters.Normal * 0.5 + 0.5;
    }
    else if (MB_DEBUG >= 3.5)
    {
        aParameters.PixelColor.rgb = aParameters.SurfaceValues.rgb;
    }
}

#endif
