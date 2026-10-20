# Unreal reference material

Status: in-progress

## Problem Statement

The user now requests the graph in Docs/ShaderUnreal.png reproduced in MM_Standard_WithSnow. This replaces the earlier derivative-based snow design as the delivery target; retain the old MM_ShaderTest prototype.

## Solution

Reproduce the connected Base Color and Normal graph: flatten the sampled tangent-space normal toward the unperturbed surface normal, use the resulting pixel normal's world-up component, multiply by 4.5, add -2.5, saturate, then lerp RGB 0.2 to RGB 0.9. Use AGP's +Y in place of Unreal's +Z. The texture's normal detail must affect snow coverage, and Flatness 1 must remove that detail.

## User Stories

1. As a material author, I want my reference graph's color and normal operations reproduced in the named AGP material.
2. As a material author, I want mapped normal detail to shape the snow boundary and Flatness to flatten the normal.

## Implementation Decisions

- Use automatic name-based shader lookup and existing normal-map texture binding.
- Add the unperturbed world vertex normal to the material pixel hook's inputs, initialized in all three pixel programs. This is an internal HLSL struct, not a GPU buffer or new interpolator.
- Reuse the existing normal map reconstruction/TBN transform. Flattening is a linear blend before normalization; its world-space equivalent blends mapped normal toward the normalized unperturbed vertex normal.
- Expose MB_Flatness, default 0; imported parameter name Flatness. The screenshot labels the Unreal parameter Flatness (S); this alternate label is not currently exported in the scene. Parenthesized Unreal names are not valid HLSL names and the current importer does not strip parentheses.
- The graph uses constant base colors, so do not multiply by exported base color or albedo texture. Preserve alpha and the existing shading pipeline.
- Existing metallic/roughness texture handling is unchanged; this task implements the connected Base Color and Normal nodes. No noise/tiling logic appears in the reference, so do not invent it from older exported material parameters.

## Testing Decisions

- Compile the actual new material through Surface_VS, GBuffer_PS, Lit_PS, and Unlit_PS with fxc shader model 5.0. Compile the existing MM_ShaderTest and default material as regression coverage for the added hook input.
- Build Debug x64 Game.sln and confirm deployment matches source.
- Runtime scene: lvl_shader_testing_tarik/Lvl_Shader_Testing_Tarik_Level.json, now already assigned MM_Standard_WithSnow by the user. Check 0.2 bare color and 0.9 snow in Albedo view; final-lit values depend on lighting/tonemapping. Normal detail shapes the boundary; Flatness 1 removes normal-map detail while preserving mesh curvature; move camera and rotate mesh to verify world-up behavior.
- Visual equivalence remains pending if UI verification is stopped or unavailable.

## Out of Scope

Unreal edits, unrelated scene/project changes, noise/tiling/snow textures, renderer redesign, and changes to the earlier prototype or audio fix.
