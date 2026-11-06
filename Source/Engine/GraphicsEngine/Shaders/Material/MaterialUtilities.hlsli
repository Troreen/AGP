#ifndef MATERIAL_UTILITIES_HLSLI
#define MATERIAL_UTILITIES_HLSLI

#include "Common.hlsli"

// Read the scene's sun direction directly from the shared frame data.
// It points toward the sun in world space; zero means the sun is absent or off.
float3 Sun_DirectionToLight()
{
    return FB_SunDirectionToLight;
}

// Point from a place on the surface toward the camera, using world coordinates.
// At the camera itself there is no direction, so return zero instead of dividing by zero.
float3 View_DirectionToCamera(float3 aWorldPosition)
{
    const float3 toCamera = FB_CameraPosition - aWorldPosition;
    return toCamera * rsqrt(max(dot(toCamera, toCamera), 1.0e-8f));
}

// Make a mask that grows stronger when looking along the edge of a surface.
// Both directions must be unit length and in the same space; view points toward the camera.
// Base reflectance is the head-on value (0 to 1). A larger exponent narrows the bright rim.
// An exponent of 5 gives the usual Schlick curve; materials can choose a different rim width.
float Fresnel_Schlick(float3 aNormal, float3 aViewDirection, float aBaseReflectance, float anExponent)
{
    const float facing = saturate(dot(aNormal, aViewDirection));
    const float base = saturate(aBaseReflectance);
    return base + (1.0f - base) * pow(1.0f - facing, max(anExponent, 0.001f));
}

// Keep a texture's repeat size fixed in screen pixels, regardless of window size.
// For example, (512, 512) repeats once every 512 pixels. Use a wrapping sampler to repeat.
// Pixel positions start at the render target's top-left; pixel centers include a half pixel.
float2 UV_ScreenTexture(float2 aPixelPosition, float2 aTextureSize)
{
    return aPixelPosition / max(aTextureSize, float2(1, 1));
}

#endif
