# Splat map MVP: preparation checklist and agent handoff

Status: live splatmap testing scene and connected material inspected; painted-weight export remains unresolved. Updated 2026-10-22. This is the owner's planning record and the next agent's continuation context, not a finalized implementation specification.

## Latest live research: splatmap testing scene

The parallel investigation and independent cross-reviews are recorded in
[investigation synthesis](../.scratch/terrain-splatmaps/investigation-summary.md)
and [execution plan](../.scratch/terrain-splatmaps/investigation-plan.md).
Owner decisions now preserve Dunes -> Rough -> Cliff order, fill uncovered
areas with Dunes, use linear ORM, constrain noise to existing transitions, and
match the native Landscape's world-space 8m surface placement.

The owner clarified that stored import Yaw=-90 is intentional and must remain.
Rotation does not corrupt UVs. The separate Landscape-FBX V-convention mismatch
is unwanted. An approved scratch-FBX correction and unsaved temporary import
preserved yaw -90, retained identical geometry/topology, and verified corrected
UV0/UV1 at all 18816 mesh corners. See
[test result](../.scratch/terrain-splatmaps/audit-uv-corrected-yaw-minus90.json).
Correct asset preparation/export/import is the preferred fix; do not compensate
for this asset-specific problem in HLSL. Final appearance remains unverified.
See [cause and source evidence](../.scratch/terrain-splatmaps/uv-orientation-cause.md)
and [temporary import measurement](../.scratch/terrain-splatmaps/audit-zero-rotation-import.json).
The scratch-FBX test is complete; the DDS round-trip still awaits approval.

The current Unreal map is `/Game/Levels/Test_Levels/SP/Lvl_Splatmap_Testing` in
`C:/Users/tarik.bergstrom/Perforce/AGP-Content/Unreal/P5G4_KnightHeart/`.
Both its native Landscape and converted `SM_Landscape_Test` use
`MI_Landscape_SplatMapTesting`, parent `/Game/Materials/MM_Landscape`.
The connected graph confirms Snow_Dunes, Snow_Rough and Snow_Cliff with all nine
surface textures. All three layer inputs currently use alpha blending.
DEBUG=1 substitutes red, green and blue respectively. Those debug colors are
not evidence of internal Landscape weightmap channel packing.

Painting weights are present in the Landscape data. The current JSON at
`Content/ExportedScenes/lvl_splatmap_testing/Lvl_Splatmap_Testing_Level.json`
contains the converted mesh/material and DEBUG parameter, but no weights or
mask transform. Reusing a Landscape material on the converted mesh does not
itself transfer Landscape painting. The connected surface function currently
uses UV0 divided by 8; world-space tiling and noisy weights remain intended AGP
behavior rather than verified features of this Unreal graph.

Unreal exposes `Landscape.render_weightmaps` to render requested layers into
chosen RGBA channels of a render target. An owner-approved temporary 512x512 bake
succeeded, producing a diagnostic RGB image (Dunes, Rough, Cliff) under `.scratch`.
No source assets or level were saved. Verify numeric color handling and mask registration before settling
the transport contract. No code or material changes are authorized; ask the
owner before testing beyond inspection. Earlier sections below retain historical
museum/offline context and must be read against these newer observations.

Evidence and remaining gaps: [live Unreal investigation](../.scratch/terrain-splatmaps/unreal-live-investigation.md)
and [current JSON/exporter investigation](../.scratch/terrain-splatmaps/current-export-research.md).

## Goal and agreed requirements

Deliver a basic splat-map material implementation quickly, with reasonable visual quality. The required behavior is material distribution/blending controlled by a splat map, with noise applied to the blend weights for more natural transitions. Example surfaces are snow, rock and grass.

The target is three surface layers, each using base color, ORM and normal maps: nine surface textures in total, plus the blend mask(s) and any texture-based noise. Plan to blend all three surface properties. The exact layer identities and texture assignments must come from the museum material, rather than assuming the example surfaces above.

Use world-space planar coordinates for the surface textures, with one repeat per 8 metres. The requested plane is Unreal world XY (the ground plane); verify its corresponding axes in AGP rather than copying `.xy` directly into HLSL. Plan for world-space splat-mask sampling as well, so terrain and ground floaters can share the shader and aligned material distribution. This is an intended implementation path whose alignment still needs verification against the museum setup.

Terrain geometry is a separate input: the exported mesh should already contain hills and valleys. Texture-height-based material blending is an optional bonus, deferred from the MVP. Heightmap terrain generation and gameplay height queries are not requested.

Owner clarification: the existing geometry workflow is terrain authored in Unreal, converted/prepared as FBX through external software, then reimported into Unreal and placed in scenes as mesh assets. Reuse that workflow and the ordinary AGP mesh import route. The external tool/steps and exact museum mesh are still to be identified; native Landscape geometry export is not a new feature requirement. A native Landscape may remain in the source map alongside converted meshes.

AGP-Content is now present beside AGP on this device, including the Unreal project and exporter source. Unreal is still downloading, so editor inspection, fresh exports and Unreal reference captures are pending. File/source investigation can continue now. This update authorizes planning only; do not start implementation automatically.

## Current setup and remaining checks

- [x] **Content and exporter source available locally.** AGP-Content contains the Unreal 5.7 project, TGA plugin source, runtime exports and texture/mesh assets. Plugin compilation and operation have not been tested here.
- [x] **Representative setup identified by the owner.** The graphics teammate reports setting up realistic splat-map terrain in the museum scene. Inspect this existing setup before requesting any replacement asset package. Its presence and configuration in the local map remain unverified in the editor.
- [x] **Layer count and surface-map target established.** Three layers, each with base color, ORM and normal maps.
- [x] **Museum candidates traced without the editor.** Map package strings reference `/Game/Materials/MM_Landscape` and `Snow_Cliff`, `Snow_Dunes`, `Snow_Rough` LayerInfo assets. The master references all nine corresponding textures, and their runtime DDS files are present. Active material connections/instance overrides and painted appearance still require Unreal verification.
- [ ] **Locate the exact museum terrain and material.** Trace the FBX-derived terrain mesh and its material instance/master through the established reimport workflow. If the demonstrated splat setup is still on a native Landscape, establish how its mask/material inputs transfer to the converted mesh. Confirm the teammate's changes reached this checkout.
- [ ] **Identify the mask and mapping.** Record layer/channel meanings, world-space origin, bounds/scale and orientation, and whether painted weights need a separate bake/export step. Verify the Unreal XY-to-AGP ground-plane conversion and world units. Target world-space sampling shared with ground floaters.
- [ ] **Resolve the nine surface textures.** Record exact source/runtime paths, ORM packing and normal conventions from the selected material. Available snow/grass assets are candidates, not verified assignments.
- [ ] **Choose noise.** Inspect the intended material and local noise candidates; select a suitable texture or procedural source, mapping, scale and strength.
- [ ] **Verify the existing mesh workflow and mask delivery.** Identify the already converted terrain FBX, its scene reference and placement. Document the external conversion/reimport steps and how mask/material inputs accompany that mesh. Reuse this route rather than adding native Landscape geometry export.
- [ ] **Capture a reference.** Save the museum terrain with identifiable material regions and a recognizable landmark for alignment comparison.
- [ ] **Finalize the implementation specification.** Resolve the remaining transport and blending details from this setup; preserve three-layer color/ORM/normal scope.

## Collection record

| Item | Path, answer or remaining unknown |
| --- | --- |
| Target project checkout and branch | `C:/Users/tarik/Documents/GitHub/AGP`, branch `splatmap-blending`; sibling `../AGP-Content` |
| Unreal version / exporter location and revision | Project and plugin declare 5.7; `../AGP-Content/Unreal/P5G4_KnightHeart/Plugins/TGAUnrealPlugin/Source/`; content HEAD observed as `c9465d8`; plugin descriptor version 1.0. Editor still downloading; plugin operation unverified. |
| Current importer location | `Source/Engine/GameFramework/UnrealSceneImporter/UnrealSceneImporter.cpp` |
| Source Unreal scene / export steps | `../AGP-Content/Unreal/P5G4_KnightHeart/Content/Levels/Test_Levels/SG/Lvl_Asset_Museum.umap`; exact terrain actor/material and repeatable export steps pending editor inspection. |
| Exported scene and terrain mesh | `../AGP-Content/Content/ExportedScenes/lvl_asset_museum/Lvl_Asset_Museum_Level.json`; no identifiable three-layer splat material in this JSON. Correct terrain mesh/export pending verification. |
| Splat mask(s), channel meanings and mapping | Not identified yet; do not assume RGB channel assignments or UV mapping. |
| Surface coordinate mapping | World-space ground-plane projection: Unreal XY maps to AGP ZX through the scene importer. One repeat per 8 metres; expected divisor is 800 for centimetre-authored content, pending measured render-mesh verification. Apply to each layer's base color, ORM and normal maps. |
| Mask coordinate mapping / ground floaters | Target world-space mapping with shared origin, extent and orientation across terrain and floaters. Exact mask transform and edge behavior pending museum inspection. |
| Surface textures and packing conventions | Candidate master references `T_8M_Snow_Cliff`, `T_8M_Snow_Dunes`, `T_8M_Snow_Rough` sets; all nine DDS files exist under `../AGP-Content/Content/Textures/`, 1024 x 1024 (base color BC7 sRGB, ORM BC7 linear, normals BC5U). Active instance assignments, packing semantics and normal convention require editor confirmation. |
| Noise source | Local noise textures exist; suitability and intended source unverified. |
| Minimum simultaneous layer count | Three. Two-layer testing may be diagnostic only, not the delivered target. |
| Visual quality requirements / reference screenshot | Realistic museum setup is the reference target; blend base color, ORM and normals with noisy weight transitions. Screenshot pending Unreal. |

## Minimum reusable test setup

Use the museum terrain, or a small reproducible section derived from it, with visible slope, three material regions and gradual transitions. Keep an asymmetric landmark or corner to reveal flipped mapping. Keep the same mesh and surface assets while editing the mask.

Prepare or generate these variations on the target device; they do not require separate graphics deliveries:

1. Distinct solid material regions to check assignment and mapping.
2. A smooth transition to check basic weight blending.
3. The same transition with noise off/on to judge the change in blend weights.
4. A measured 8-metre span to confirm one surface-texture repeat, including after mesh translation/rotation/scale. Texture placement should follow world position.
5. A ground floater overlapping a recognizable terrain layer boundary, using the same shader, textures and world-to-mask transform. Verify matching distribution and texture phase despite different mesh UVs; include a rotated floater to check normal-map orientation.

A two-surface diagnostic may help isolate a problem, but completion requires all three layers with their base-color, ORM and normal maps. Use existing texture resolutions initially. Runtime mask/noise textures must be treated as linear data; the checked-in engine texture route currently loads DDS.

## Quality and scope guidance for the next planning session

Keep the implementation small while blending the three supplied layers' base color, ORM and normals. Verify packing, color-space handling and normal blending against the museum reference. A base-color-only result is an intermediate check, not completion.

Prefer one material using existing mesh, texture and parameter plumbing. Preserve the painted distribution and give noise a visible, adjustable effect on transitions. Noise strength zero should allow a direct comparison with the original mask. Keep terrain-wide mask mapping separate from repeating surface texture tiling.

### World-space mapping approach

- Derive a shared horizontal coordinate from world position, converted to a documented axis convention and unit scale. Surface UVs conceptually equal ground-plane position in metres divided by 8, with a common phase/origin across meshes. Apply the same mapping to each layer's base color, ORM and normal textures, using wrapping samples.
- Derive mask UVs separately from the mask's world origin and coverage extent, with any orientation/axis flips recorded explicitly. The mask covers the authored terrain region; it does not automatically repeat every 8 metres. Define clamp/outside-coverage behavior after inspecting the source.
- Give terrain and floaters the same world-to-mask transform and texture phase. Avoid per-mesh bounds or local UVs as the default mapping, since these would shift the distribution between meshes. If noise uses position, share its mapping too.
- Verify the normal-map basis for the projected UVs. Mesh tangent frames may not match world-projected texture directions, especially on rotated floaters; define the projected-normal transformation rather than assuming ordinary mesh-UV normal sampling remains correct.
- Start with planar ground projection. Steep-slope stretching is an observable limitation to assess against the museum reference; triplanar projection is not an automatic scope addition.

World position is already exposed to material code, so the coordinate formulas should be small. Source inspection confirms that the scene importer maps Unreal XYZ to AGP YZX: faithful Unreal XY projection therefore uses AGP `WorldPosition.zx`. The importer preserves numeric translation scale (`UnrealUnitScale = 1.0f`); the expected 8-metre repeat is 800 world units for centimetre-authored content. Verify this against measured render geometry because the prebuilt FBX importer's internal conversions are not available here. The remaining work is establishing the mask transform, confirming the combined mesh/scene scale and handling projected normals correctly.

No advanced terrain system, arbitrary layer framework, editing UI or hot reload is needed. Height blending remains deferred. Layer count is three; exact channel layout, noise function, controls and shader formulas still need definition. Also settle weight normalization, zero-weight fallback and whether noise can introduce an otherwise absent layer.

## What we already know from this device

- The existing FBX route imports positions, UV0/UV1, normals, tangents and material slots. Geometry and mapping fidelity for the actual terrain have not been tested.
- The current importer carries scalar, vector and named texture parameters. Material hooks expose UVs and world position, so explicit splat inputs may fit the existing route.
- AGP uses its own HLSL material selected by master-material name; it does not translate Unreal's material graph.
- The material interface has a 16-texture budget and a 128-byte parameter block. Allocate within the existing system before proposing architectural changes.
- The earlier inventory covered 12 exports in the older checkout. Its 11 native Landscape records had only root metadata; two landscape-named static meshes used the ordinary mesh path. This is historical evidence, not a fresh census of AGP-Content or proof about the teammate's museum setup.
- No explicit splat contract or noisy splat shader was found. Existing snow/noise parameter names do not establish implementation.
- Some exported texture names do not match local assets, and missing meshes/materials can use fallbacks. Successful scene loading alone does not prove the intended data was used.
- The exporter source is now accessible. The serializer carries mesh references, material texture references and scalar/vector parameters, but has no registered Landscape handler or built-in painted-weight extraction. Its FBX export routine exports navmesh, not visual terrain. The owner's external FBX conversion/reimport workflow already addresses geometry; the lack of native Landscape geometry export is not a blocker for that route. Verify how splat weights are delivered with the converted mesh.
- The museum map contains Landscape/weightmap metadata, the three snow LayerInfo references and `/Game/Materials/MM_Landscape`. These package-string findings support the reported setup without verifying its active graph or appearance.
- `ExportAssets.bat` handles existing FBX and standard surface PNG naming patterns, not arbitrary mask/noise PNGs or Unreal-internal weightmaps. `texconv.exe` is absent locally. Plan an explicit linear mask conversion path; existing DDS surface sets need no reconversion.
- The current museum JSON lists only `MM_Snow`, `MM_Standard_WithSnow`, `MM_Standard` and `WorldGridMaterial` as master materials. It does not expose an identifiable three-layer splat setup. This may be an older/incomplete export; it does not disprove the setup in the Unreal map.

Current evidence: [museum investigation](../.scratch/terrain-splatmaps/museum-investigation.md) and [world-space mapping investigation](../.scratch/terrain-splatmaps/world-mapping-investigation.md). Historical evidence: [pipeline investigation](../.scratch/terrain-splatmaps/pipeline-investigation.md), [export inventory](../.scratch/terrain-splatmaps/export-inventory.md), and [terrain concepts research](../.scratch/terrain-splatmaps/research.md).

## Next agent: resume here

1. Read this checklist and [workflow state](../.scratch/terrain-splatmaps/workflow.md). Read the repo's AGENTS.md and use the repo-local agp-feature workflow.
2. Inspect Git status, current branch, supplied paths, actual importer/exporter and representative assets. Preserve existing work. The planning branch was created as `splatmap-blending` from `shader-testing-scene` at `02886bf09c023aade3085315e6fb13fde1104e19`; do not assume another device has the same checkout state.
3. Review the owner's collected answers. Verify facts from the supplied files before asking questions. Do not assume the older checkout's schema matches the newer toolchain.
4. Continue file/source inspection while Unreal downloads. Once available, inspect the museum setup and establish exact mesh/mask/texture mapping and the smallest necessary export/import changes. Three layers with base-color/ORM/normal blending are the planning target; do not reopen those questions without new evidence or an owner scope change.
5. Finalize a short implementation spec and task breakdown from those decisions. Proceed to implementation when the owner requests it; this handoff is not implementation authorization.
6. Later verification should show the correct mesh/mask alignment, expected layer regions, a visible noise-off/on difference and acceptable lighting. Use a small scene and screenshots, build the relevant projects, and reuse existing test seams where useful. Do not invent a large testing framework.

No engine code has been changed or runtime splat behavior verified for this feature. The working tree was clean before this planning update. Leave this update uncommitted and unstaged unless the owner requests otherwise.
