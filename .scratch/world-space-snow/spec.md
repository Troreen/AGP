# World-space snow

Status: in-progress

## Problem Statement

The shader-test mesh needs white snow on its upward-facing surfaces.

## Solution

Use interpolated world vertex positions to derive geometric surface direction. Blend the existing base color toward white as the surface faces world up, with a soft edge between bare sides and covered tops. Coverage follows world up when an object rotates or translates.

## User Stories

1. As a scene author, I want the shader-test material's tops to look snowy.
2. As a scene author, I want sides and undersides to retain their base appearance.
3. As a scene author, I want rotating the mesh to change which surfaces collect snow.

## Implementation Decisions

- Use the exported master material name MM_ShaderTest and existing name-based material shader lookup.
- Use world-space position derivatives, orient the resulting geometric normal against the existing world normal, and compare with +Y.
- Use a smooth coverage ramp from upward dot 0.25 (bare) to 0.75 (white).
- Retain base color, alpha, lighting, shadows, and existing roughness/metalness parameter names. No vertex displacement.
- Keep the effect local to this master material.

## Testing Decisions

- Build Debug x64 Game.sln with v145; material HLSL compiles at runtime, so a C++ build alone is insufficient.
- Compile the material through the existing surface vertex, lit pixel, and G-buffer pixel programs; verify unlit compatibility as applicable.
- Runtime acceptance scene: lvl_shader_testing_tarik/Lvl_Shader_Testing_Tarik_Level.json in Bin/Debug/Game.exe. View the sphere from above and below using F1/debug camera, right mouse + WASD/Space/Ctrl. Top must be white, sides transition smoothly, bottom retains gray. Lighting and shadows must still apply.
- Rotate the sphere/test mesh 90 degrees and translate it: snow remains on world-up surfaces; verify from different camera angles. Compare an ordinary material to ensure it is unaffected.
- Capture runtime evidence where possible. A human visual check remains required if UI capture/control is unavailable.
- No new CPU test seam that merely repeats HLSL math.

## Out of Scope

Snow textures, accumulation over time, displacement, weather, occlusion by other meshes, height cutoffs, new renderer architecture, and Unreal material graph edits.

## Further Notes

Requested MM_Shader_Test corresponds to MM_ShaderTest in the existing exported scene. Existing build deployment copies engine shaders automatically.
