# Splat material pipeline investigation

Status: needs-info

Investigation date: 2026-10-19. Scope: current checkout and bundled exporter manual. The user confirms the newer Unreal/Perforce tooling is inaccessible on this device. Its capabilities remain unverified.

## Confirmed responsibility split

The graphics team's required behavior is splat-map material blending with noise applied to blend weights for natural transitions among surfaces such as snow, rock and grass. Texture-height-based material blending is an optional bonus. Terrain geometry is a separate input dependency: if a suitable terrain mesh is exported, AGP does not need to reconstruct elevation for this feature. Ground-height queries, collision, displacement and heightmap terrain generation are not established requirements.

## Current end-to-end route

1. Unreal scene JSON supplies actors, transforms, static mesh references, ordered material slots, master material metadata, and material parameters.
2. `UnrealSceneImporter` reads mesh references and supported parameters into `SceneData`.
3. `BuildWorld` resolves FBX assets through `AssetRegistry`, creates mesh components and applies material assignments.
4. A master material name selects a corresponding AGP HLSL file. Scalar/vector values and named texture resources bind to that shader.
5. Material pixel code can read geometry UVs/world position and replace surface color, normal, packed surface properties and emission.

This is an existing general material route that could carry explicit splat inputs. It is not proof that the external exporter generates those inputs automatically.

## Evidence and limits

| Area | Evidence in this checkout | Consequence for splat materials |
| --- | --- | --- |
| Exporter contract | Bundled programmer manual describes Type 1 mesh name, content path, and ordered material list; parameter types include scalar, vector and texture. It does not document Landscape terrain geometry or painted layer map export. | Explicitly authored mesh/material parameters fit the documented path. Automatic Landscape map extraction is unverified. |
| Scene import | Importer reads Type 1/2 meshes and scalar/type-1 vector/type-3 texture parameters. Runtime conversion retains parameter names. | Mask texture references, per-layer tiling and noise controls can use existing types if the exporter supplies them. Unsupported parameter types are not a substitute for these inputs. |
| Texture identity | For texture parameters the importer takes the filename stem of `AssetImportPath`; it does not retain the complete Unreal path as the lookup identity. Registry aliases are filename-based and can collide. | Use unique resolvable asset names initially; validate that each shader resource receives the intended DDS. A source PNG filename stem may resolve to a DDS of the same stem, but this is lookup behavior, not automatic image conversion. |
| Mesh data | `MeshAsset` imports FBX elements, indices and material indices. Vertex conversion copies position, UV0, UV1, normal, tangent and first vertex color set. | Useful inputs exist for mask alignment and repeating surface textures. Actual terrain UV coverage, seams, dimensions and orientation still need a runtime fixture. |
| Native Landscape | No dedicated Landscape component exists in importer types or `SceneData`. Inventory of local JSON finds root-only Landscape actors. | A Landscape actor's presence is not evidence that terrain is rendered. Mesh export/conversion or a separate terrain path is required if geometry is absent. |
| Texture formats | Registry registers `.dds`; RHI uses `CreateDDSTextureFromMemory`. | Deliver masks/noise/surface images as supported DDS, or explicitly add an offline conversion/import step. PNG files alone do not establish runtime support. |
| Shader implementation | Master name selects `<name>.hlsli`; missing shader uses a fallback. Shader reflection supplies named resource bindings. | Author an AGP splat shader. Exported Unreal material parameters do not reproduce the Unreal graph. Confirm actual shader selection and logs in acceptance checks. |
| Material settings | Scalars/vectors bind after spaces become underscores and an `MB_` prefix is applied; texture parameters first bind by their parameter name. | Agree exact parameter and resource names. Generic strings or tags are not necessary for ordinary numeric controls and texture references. |
| Budget | Material interface allows 16 texture slots and a 128-byte parameter block. Standard surface pixel shader declares t0–t3; material guide reserves extra slots starting at t4. | Account for standard and extra resources together. Three sets of color/normal/ORM plus mask/noise appear feasible within 16 slots with explicit allocation; arbitrary layers are not promised. |
| Existing snow | Current snow shaders derive coverage from upward-facing normals and blend color/properties. No inspected shader reads a splat mask and noises its weights. | Useful prior material work exists, but current slope snow is not the requested system. Noise-named exported values alone do not prove a shader uses them. |
| Fallback behavior | Missing mesh/material assets may produce a fallback cube or material. Parameter failures log warnings and may leave defaults. | A scene merely loading successfully is insufficient evidence. Check asset resolution, shader compilation and visible layer identity. |

Source pointers:

- [Exporter manual](<../../Docs/Unreal Level Exporter_Usage for Programmers _ TGA Wiki.pdf>) (single PDF page; text extracted and read).
- [Importer parser and conversion](../../Source/Engine/GameFramework/UnrealSceneImporter/UnrealSceneImporter.cpp).
- [Importer component/parameter types](../../Source/Engine/GameFramework/UnrealSceneImporter/UnrealSceneStructs.h).
- [Scene records](../../Source/Engine/GameFramework/Scenes/SceneData.h).
- [World construction and parameter binding](../../Source/Engine/GameFramework/Scenes/SceneWorldBuilder.cpp).
- [FBX mesh asset](../../Source/Engine/GameFramework/AssetHandling/MeshAsset.cpp) and [vertex conversion](../../Source/Engine/GameFramework/AssetHandling/AssetLoadingHelper.cpp).
- [Asset registration and aliases](../../Source/Engine/GameFramework/AssetHandling/AssetRegistry.cpp).
- [DDS loading](../../Source/Engine/GraphicsEngine/RHI/RenderHardwareInterface.cpp).
- [Material interface and budgets](../../Source/Engine/GraphicsEngine/Materials/MaterialInterface.h).
- [Shader resource reflection](../../Source/Engine/GraphicsEngine/GraphicsEngine.cpp).
- [Pixel material inputs](../../Source/Engine/GraphicsEngine/Shaders/Material/GBuffer_PS.hlsl).
- [Material authoring guide](../../Docs/MaterialShaderGuide.md).
- [Existing slope snow](../../Source/Engine/GraphicsEngine/Shaders/MM_Standard_WithSnow.hlsli) and [rock/snow material](../../Source/Engine/GraphicsEngine/Shaders/MM_BrownRock08.hlsli).
- [Importer handoff limitation](../../Docs/ImporterHandoff.md).

## Data needed from the actual terrain export

This is a handoff checklist, not a finalized schema or a requirement to expand the exporter before inspecting its output.

| Needed input | What to establish | Already representable locally? |
| --- | --- | --- |
| Renderable terrain geometry | FBX file, scene mesh reference, transform, units/orientation and material-slot association | Yes through the static mesh route; actual export fidelity unverified |
| Geometry-to-mask mapping | UV channel or documented world-to-mask transform, terrain bounds, orientation, seams/tile offsets where present | UV0/UV1/world position exist; mapping values can use numeric parameters |
| Blend weights | Mask image(s), channel-to-layer names, participating channels, linear interpretation, precision/compression and blending semantics | Texture parameters can carry images; no dedicated automatic Landscape weight extraction shown |
| Surface layers | Snow/rock/grass texture assignments, channel packing conventions and tiling values; decide required normal/roughness fidelity | Named textures and numeric controls exist; author matching HLSL |
| Noise | Either a specified procedural function or a linear noise texture, plus coordinate mapping, frequency, strength and any seed/offset | Existing shader inputs and numeric/texture parameters can represent these; behavior still needs definition |
| Material identity | Master material/shader name and exact parameter names | Yes, subject to naming and path-resolution rules above |
| Reference | Unreal/exporter versions/settings and images of terrain shape, painted weights and final appearance | Handoff evidence, not an engine feature |
| Optional detail heights | Per-layer detail-height channel/assets and agreed interpretation | Defer until required blending is verified |

Keep mask mapping independent of surface tiling. Repeating grass many times should not repeat the terrain-wide snow/rock/grass distribution.

Noise is part of blend-weight calculation, not a requirement to alter geometry or simply add color variation. Later acceptance should compare noise strength zero with nonzero on the same masks, confirm stable coordinates, decide whether absent layers must remain absent, maintain valid weights, and define a zero-total fallback. The exact perturbation function is not selected by this investigation.

## Suggested evidence package for the other device

Export one small asymmetric terrain mesh, with a known corner/landmark and three visibly painted regions. Include the scene JSON, FBX, mask files, layer textures or diagnostic colors, material parameter listing, tool versions/settings, and Unreal screenshots. Include a smooth mixed boundary to evaluate noise. Record whether the mesh was produced from Landscape or authored separately and whether weights were exported, baked, packed manually, or generated procedurally.

On receipt, inspect the files first. Then verify geometry and a mask-as-color display in AGP before planning the final splat shader. This reveals rotation, mirroring, wrong UV channels, missing assets and fallbacks without conflating them with the blending algorithm.

## Conclusion

This checkout has a plausible existing mesh/material transport for explicit splat assets and parameters. It does not establish automatic Landscape geometry or painted-weight export, and it does not implement the requested noisy splat blending. The inaccessible newer toolchain and a representative export are the remaining external evidence needed. No renderer architecture or exporter schema changes are justified yet.

## Verification performed

Read code and exporter manual; parse local scene JSON inventory; inspect available asset paths and selected DDS headers. No FBX SDK load, game execution, visual match, exporter run, shader modification or build was performed. Findings are static support evidence, not runtime proof.
