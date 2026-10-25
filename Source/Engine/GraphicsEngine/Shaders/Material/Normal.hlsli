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

// Orient a decoded detail normal on a world-aligned projection while retaining
// the mesh's geometric slope. The detail map's +X follows projected world +Z;
// use world +X when that axis is parallel to the surface normal. Mesh UV
// tangents are deliberately unused, so rotated meshes share the same frame.
float3 Normal_ProjectWorldZ(float3 aDetailNormal, float3 aGeometricNormal)
{
    // Guard against zero-length input before normalization. The fallback is
    // world up for geometry and a flat +Z normal for the detail map.
    const float geometricLengthSquared = dot(aGeometricNormal, aGeometricNormal);
    const float3 normal = geometricLengthSquared > 1.0e-8f
        ? aGeometricNormal * rsqrt(geometricLengthSquared) : float3(0, 1, 0);
    float3 tangent = float3(0, 0, 1) - normal * normal.z;
    if (dot(tangent, tangent) < 1.0e-6f)
    {
        // Projection onto a nearly perpendicular plane has an unstable U axis.
        tangent = float3(1, 0, 0) - normal * normal.x;
    }
    tangent = normalize(tangent);
    const float3 bitangent = cross(normal, tangent);
    const float detailLengthSquared = dot(aDetailNormal, aDetailNormal);
    const float3 detail = detailLengthSquared > 1.0e-8f
        ? aDetailNormal * rsqrt(detailLengthSquared) : float3(0, 0, 1);
    return normalize(detail.x * tangent + detail.y * bitangent + detail.z * normal);
}

#endif
