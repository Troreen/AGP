# Shared material shader helpers

The reusable HLSL headers live beside the material entry points in
`Source/Engine/GraphicsEngine/Shaders/Material/`. Include them by name from a
custom material; the normal Game build deploys them to `Bin/Debug/EngineShaders/Material/`.
Premake already includes all shader headers, and the checked-in Visual Studio
project lists both new files.

## Normals

Include `Normal.hlsli` for these operations:

- `Normal_DeriveZ(normalXY)` takes XY values already converted to signed tilt
  (usually -1 to 1) and returns XYZ, like Unreal's DeriveNormalZ node. Z is
  reconstructed as the upward-facing part. If XY is too long, Z is clamped to
  zero; XY is left unchanged. Normalize the result before normal blending.
  To show the reference graph's steps, first calculate `sample.rg * 2 - 1`,
  then pass that to this helper.
- `Normal_DecodeRG(encodedXY)` reads red/green values in 0–1 and returns a unit
  tangent-space normal. Z is reconstructed as the upward-facing part. Sample
  normal textures as linear data, without an sRGB conversion.
  This is a convenience wrapper around the signed conversion, `Normal_DeriveZ`,
  and normalization, so existing materials can keep using the shorter form.
- `Normal_BlendAngleCorrected(base, detail)` combines decoded unit normals in
  the same tangent space. A flat `(0, 0, 1)` detail leaves the base unchanged.
  The maps should face up in tangent space. Degenerate input that cancels the
  result falls back to a flat normal.
- `Normal_TangentToWorld(normalTS, normal, tangent, bitangent)` converts a
  tangent-space normal to a unit world-space direction using the mesh's valid
  world-space surface axes.

The existing `GetWorldNormal` calculation in `Lighting.hlsli` is unchanged.
Custom materials can include `Normal.hlsli` when they need these extra operations.

## View, rim masks, and screen textures

Include `MaterialUtilities.hlsli`:

- `Sun_DirectionToLight()` reads the sun direction directly from the frame buffer.
  It returns a unit world-space direction toward the scene's directional light
  when its intensity is positive, or zero when the light is absent or off.
  Scenes have at most one directional light. This includes no light color,
  intensity scaling, or shadow visibility.
- `View_DirectionToCamera(worldPosition)` returns the world-space direction
  toward the camera. At the camera position it returns zero.
- `Fresnel_Schlick(normal, viewDirection, baseReflectance, exponent)` makes a
  rim mask. Pass unit directions in the same space. Base reflectance controls
  the head-on value; exponent controls the rim width. Exponent 5 is the usual
  Schlick approximation; 6 reproduces the illustrated graph's exponent.
- `UV_ScreenTexture(pixelPosition, textureSize)` divides pixel position by a
  repeat size in pixels. `(512, 512)` repeats every 512 pixels with a wrapping
  sampler. Size components below 1 are treated as 1.

The screen helper uses render-target pixels, measured from the top-left corner,
with pixel centers at half-pixel positions. It does not account for a viewport
offset or a separate output/upscaling resolution.

## Material inputs

All three pixel entry points (opaque G-buffer, forward lit, and unlit) supply
`parameters.ScreenPosition`: render-target pixel coordinates from `SV_Position`.
The sun direction belongs to shared frame data and is read through the helper,
so it is not copied into each material's parameter structure.

The sun direction is captured during the existing light setup each frame, so
rotation, removal, and intensity changes are reflected without another search.
It is supplied through the shared frame buffer before opaque materials
run, independently of the light buffer used by later lighting passes. Its C++
and HLSL layouts are kept in sync, with C++ size and offset checks.

For example, inside `Material_Pixel`:

```hlsl
const float3 view = View_DirectionToCamera(parameters.WorldPosition.xyz);
const float rim = Fresnel_Schlick(normalize(parameters.Normal), view, 0.04f, 6.0f);
const float sunMask = saturate(dot(normalize(parameters.VertexNormal), Sun_DirectionToLight()));
const float2 sparkleUV = UV_ScreenTexture(parameters.ScreenPosition, float2(512, 512));
```

These are building blocks; no snow shader, noise header, or sparkle test actor
is added. The older `Content/Shaders/Material/` shader set has a separate
interface and is not extended by this change.

## Verification

Run `powershell.exe -NoProfile -ExecutionPolicy Bypass -File Tests/GraphicsEngine/VerifyMaterialHelpers.ps1`.
This sets the script policy for that process only. It compiles
a fixture using every helper and the screen-position input through the four engine
material entry points. It checks shader integration, not rendered appearance.
Build `Game.sln` in Debug x64 to check the C++ frame buffer and deployment.
