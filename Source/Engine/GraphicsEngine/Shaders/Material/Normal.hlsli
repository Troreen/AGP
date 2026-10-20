#ifndef MATERIAL_NORMAL_HLSLI
#define MATERIAL_NORMAL_HLSLI

// Recover the missing upward part of a tangent-space normal, like Unreal's DeriveNormalZ.
// XY must already be signed tilt values, usually between -1 and 1.
// If XY is too long, keep Z at zero rather than taking the square root of a negative number.
float3 Normal_DeriveZ(float2 aNormalXY)
{
    const float z = sqrt(saturate(1.0f - dot(aNormalXY, aNormalXY)));
    return float3(aNormalXY, z);
}

// Shortcut for a normal texture stored in red and green, with 0.5 meaning no tilt.
// Read the texture as linear data. Convert to signed tilt, recover Z, and keep it one unit long.
float3 Normal_DecodeRG(float2 anEncodedXY)
{
    return normalize(Normal_DeriveZ(anEncodedXY * 2.0f - 1.0f));
}

// Lay small bumps over a larger shape without washing out either normal map.
// Both inputs are decoded, unit-length normals in the same tangent space.
// A flat detail normal (0, 0, 1) leaves the base normal unchanged.
float3 Normal_BlendAngleCorrected(float3 aBase, float3 aDetail)
{
    const float3 base = aBase + float3(0, 0, 1);
    const float3 detail = aDetail * float3(-1, -1, 1);
    const float3 blended = base * dot(base, detail) - detail * base.z;
    // Bad or downward-facing map data can cancel out; use a flat normal then.
    const float lengthSquared = dot(blended, blended);
    return lengthSquared > 1.0e-8f ? blended * rsqrt(lengthSquared) : float3(0, 0, 1);
}

// Turn a normal relative to the surface into a direction in the game world.
// Pass the mesh's normal, tangent, and bitangent in world space, in that order.
float3 Normal_TangentToWorld(float3 aNormalTS, float3 aNormal, float3 aTangent, float3 aBitangent)
{
    const float3x3 surfaceAxes = float3x3(normalize(aTangent), normalize(aBitangent), normalize(aNormal));
    return normalize(mul(aNormalTS, surfaceAxes));
}

#endif
