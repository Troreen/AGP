# Museum splat setup: investigation before Unreal installation

Date: 2026-10-22. Scope: read-only inspection of local exporter source, exported JSON, Unreal asset strings and DDS headers. No implementation, editor launch, asset conversion or runtime verification performed.

Owner clarification after inspection: terrain is authored in Unreal, converted/prepared as FBX in external software, then reimported and placed in scenes. This supplies the intended geometry through the ordinary mesh route. The exact external tool and museum FBX remain untraced. Native Landscape metadata in the map does not establish that it is the final render mesh; reuse the existing conversion workflow, and investigate how the splat mask/material travels with it.

## Findings

The local museum map contains evidence consistent with the teammate's three-layer terrain setup. It references `/Game/Materials/MM_Landscape`, the layer names `Snow_Cliff`, `Snow_Dunes`, `Snow_Rough`, and their map-specific LayerInfo assets. The material contains references to three complete texture sets with those names. All nine corresponding DDS files are present and have valid DDS headers.

These are package-reference/string findings, not a decoded Unreal material graph. They establish stronger candidates than filenames alone, but do not establish the painted appearance, active texture overrides, graph connections or mapping parameters. Verify those in Unreal.

## Local sources and assets

All content paths below are relative to `C:/Users/tarik/Documents/GitHub/AGP-Content/`.

| Item | Path / evidence |
| --- | --- |
| Source map | `Unreal/P5G4_KnightHeart/Content/Levels/Test_Levels/SG/Lvl_Asset_Museum.umap`; ASCII package strings include `/Game/Materials/MM_Landscape`, the three layer names, Landscape component/weightmap metadata and the three shared LayerInfo paths. |
| LayerInfo assets | `Unreal/P5G4_KnightHeart/Content/Levels/Test_Levels/SG/Lvl_Asset_Museum_sharedassets/Snow_Cliff_LayerInfo.uasset`, `Snow_Dunes_LayerInfo.uasset`, `Snow_Rough_LayerInfo.uasset`. |
| Candidate master | `Unreal/P5G4_KnightHeart/Content/Materials/MM_Landscape.uasset`; strings include all nine `T_8M_Snow_*` texture paths, `LB_WeightBlend`, the three layer names, `Layer 1 Tiling`, `Layer 2 Tiling`, and `/Game/Materials/MF_LandscapeLayer`. These names do not prove parameter values or graph wiring. |
| Potential instance overrides | `Unreal/P5G4_KnightHeart/Content/Materials/MI_Landscape.uasset` references the master and `T_EN_Snow_01_C`, `T_EN_Snow_01_N`, `T_EN_Snow_01_N_ORM`. Do not assume the museum uses this instance or that master defaults are the final assignments. |
| Duplicate-looking asset paths | `Content/LandscapeMaterial/MM_Landscape.uasset` and `MI_Landscape.uasset` contain references to the corresponding `Content/Materials/` paths. Resolve them in Unreal; do not choose by basename alone. |
| Runtime museum export | `Content/ExportedScenes/lvl_asset_museum/Lvl_Asset_Museum_Level.json`; 125 actor records, 111 type-1 mesh components, four type-0 scene components and one type-5 light. No terrain/landscape-named mesh reference was found. Master-material list is only `MM_Snow`, `MM_Standard_WithSnow`, `MM_Standard`, `WorldGridMaterial`. No identifiable three-layer splat setup is transported here. |

## Candidate texture sets already available

Each row represents three files in `Content/Textures/`, using suffixes `_BaseColor.dds`, `_OcclusionRoughnessMetallic.dds`, `_Normal.dds` respectively. The material package references matching Unreal texture paths under `/Game/Textures/`.

| Prefix | Base color | ORM | Normal |
| --- | --- | --- | --- |
| `T_8M_Snow_Cliff` | 1024 x 1024, BC7 sRGB | 1024 x 1024, BC7 linear | 1024 x 1024, BC5U |
| `T_8M_Snow_Dunes` | 1024 x 1024, BC7 sRGB | 1024 x 1024, BC7 linear | 1024 x 1024, BC5U |
| `T_8M_Snow_Rough` | 1024 x 1024, BC7 sRGB | 1024 x 1024, BC7 linear | 1024 x 1024, BC5U |

Verified from file headers: `DDS ` magic, dimensions, DX10 format values 99 (base color) / 98 (ORM), and legacy FourCC `BC5U` for normals. Files are approximately 1.4 MB each, not LFS pointer text. This does not visually verify image contents or prove exact ORM channel semantics/normal handedness.

The `8M` names support the owner's intended scale but do not establish shader coordinate conversion or existing material tiling values.

## Exporter capabilities and concrete gaps

Primary source: `Unreal/P5G4_KnightHeart/Plugins/TGAUnrealPlugin/Source/TGAUnrealPlugin/Private/TGALevelSerializer.cpp`.

- `Initialize`, lines 33-48, registers scene, static/skeletal mesh, light, collider, spring-arm and camera handlers. No Landscape component handler is registered.
- `SerializeComponent`, lines 432-457, and `CanSerializeComponent`, lines 699-704, use exact-class handler lookup or `UTGASerializableInterface`. There is no automatic superclass fallback here. Unsupported Landscape components do not gain mesh export through the generic scene handler.
- `SerializeMaterial`, lines 275-313 and 339-403, exports scalar/vector/texture parameters and texture name/path/source-import references. This can carry an explicitly assigned mask and numeric mapping values. It does not bake Unreal graph operations or produce image files.
- `HandleMeshComponent`, starting line 715, transports mesh references and material assignments; it is not a native Landscape tessellation/weight extraction path.
- Searches across the local plugin source found no Landscape, weightmap or splat-specific implementation. The FBX exporter in `TGAUnrealPlugin.cpp:334` onward is explicitly `ExportNavMeshToFBX`, not visual terrain export. Custom Blueprint serialization remains possible in principle; no working museum terrain delivery has been demonstrated through it.

Conclusion: this local plugin has no built-in native Landscape geometry/painted-weight exporter, but the owner confirms an existing external FBX conversion/reimport workflow for geometry. An already converted mesh can use the ordinary scene mesh/material route; native Landscape geometry support is not required. Identify the museum FBX and its scene assignment, then verify the mask source and transport. Native Landscape weights are not automatically transferred merely by referencing the FBX. Do not automatically build a new exporter or terrain system.

## Offline asset conversion limitation

`ExportAssets.bat:32` copies existing source FBX files. Line 55 copies only PNGs named as base color, `_C`, ORM or `_Normal`; lines 66-104 convert those categories to DDS. A newly exported mask/noise PNG with an ordinary mask/noise name will not be copied or converted by this procedure. It also does not extract `.uasset` contents or Landscape-internal weightmaps.

`texconv.exe` is absent beside the script on this machine. Existing DDS files can still be inspected/used; converting new images needs the converter and an explicit linear-data conversion path. No script was run or changed in this investigation.

## Proposed material inputs to settle in the spec

This is a planning contract, not a finalized shader interface or code change:

- Three named sets of color, ORM and normal textures, using the museum's verified active assignments.
- Explicit linear mask texture(s), named channel-to-layer order, normalization/zero-weight rules, and whether noise can introduce an absent layer. Do not infer channel assignments from the order of layer names.
- World-to-mask origin, coverage extent and orientation, shared by terrain and floaters. Define behavior outside coverage. Landscape weightmaps may be per-component/internal; do not assume one global RGB mask is already on disk.
- A common world-space surface projection with an 8-metre repeat, independent of mask extent; projected-normal basis and coordinate conversion as described in the separate mapping investigation.
- Noise source and shared mapping, strength and frequency. Strength zero preserves the unmodified-mask comparison.

Budget check: `Source/Engine/GraphicsEngine/Materials/MaterialInterface.h:69-75` provides 128 parameter bytes and 16 texture slots. `Source/Engine/GraphicsEngine/Shaders/Material/GBuffer_PS.hlsl:6-9` already declares slots t0-t3. Nine explicit surface textures plus one mask and one noise texture in t4-t14 would fit arithmetically, leaving t15, but this is not yet a chosen binding layout; default surface sampling and actual reflection/binding must be accounted for. Avoid an arbitrary-layer framework.

## First editor session

1. Open the museum and identify the FBX-derived terrain mesh, its transform/bounds and active material. Distinguish any retained source Landscape from the placed converted mesh. Locate which object demonstrates the teammate's splat setup.
2. Inspect the connected graph/instance and resolve the three active texture sets. Confirm ORM channels, normal convention, world projection and existing tiling values.
3. Identify how blend weights are applied to the reimported mesh. Reuse any existing mask delivery; if weights still reside only in Landscape data, determine a bake/export step. Record channel order and world transform.
4. Trace the existing terrain FBX through external preparation, Unreal reimport, scene placement and runtime lookup. Verify placement and surface basis; do not introduce a native Landscape geometry exporter.
5. Capture an identifiable reference and preserve geometry/maps/exported JSON in AGP-Content. Then finalize the narrow implementation spec and visual checks. Implementation still requires the owner's request.
