# Custom shader inputs
Status: in-progress

## Problem Statement
Individual material HLSL includes cannot reliably modify surface properties or access useful existing mesh inputs. Unreal custom texture parameters are bound using asset names.

## Solution
Keep computations in individual master-material .hlsli files and reuse Unreal-exported scalar, vector and texture parameters.

## User Stories
1. As a shader author, I can change AO, roughness and metallic in the pixel hook and see the result in forward and deferred lighting and debug views.
2. As a shader author, I can use elapsed scene simulation time for animation.
3. As a shader author, I can access vertex color and UV1 independently in pixel computations.
4. As a material author, I can bind custom textures by exported parameter name, independent of asset name.
5. As a material author, I get consistent MB_ prefix and space-to-underscore conversion for master and instance values.

## Implementation Decisions
- First recommended batch only; vertex displacement changes and opacity masking are excluded.
- Preserve SurfaceValues packed AO/roughness/metallic interface; consume modified values after the hook with existing lighting clamps.
- Add VertexColor and UV1 to pixel parameters and initialize every hook caller.
- Time is accumulated from World.Update's sanitized simulation delta, reset on World.Clear/new world, copied through RenderSnapshot to FrameBuffer, including shadow draws.
- Custom textures prefer normalized exported parameter-name reflection binding; retain recognized standard texture suffixes and legacy asset-name lookup as fallbacks.
- No new Unreal export schema. Individual shader includes remain the computation boundary.

## Testing Decisions
- Existing World public update/snapshot seam for time progression, invalid delta, repeat snapshots and reset.
- Material compilation/reflection and runtime shader test scene for surface changes, vertex color, UV1, time and texture binding; existing tests where device initialization is feasible.
- Build Game.sln Debug x64 and relevant GameFramework tests. Visual acceptance requires observed runtime evidence; compilation alone is insufficient.

## Out of Scope
Opacity masking, new shading models, vertex-hook expansion, general renderer rewrite, staging and commits.

## Further Notes
Runtime engine shaders come from the configured EngineShaders folder copied from engine source. Content shader copies are a separate legacy set; adapt relevant interfaces without migrating that renderer.
