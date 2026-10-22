# Current splatmap export research

Observed 2026-10-22. Research only; no code, settings, assets or exports changed. This note covers local files and exporter source. Live Unreal material graph inspection belongs to the companion editor investigation.

## Current scene export

The current target is `Lvl_Splatmap_Testing`, superseding the museum-only assumptions in [the planning handoff](../../Docs/SplatmapMVPPlan.md). Its JSON exists at [AGP runtime export](../../Content/ExportedScenes/lvl_splatmap_testing/Lvl_Splatmap_Testing_Level.json). File observation: 2,799 bytes, last write 2026-10-22 15:28:53 local time; SHA256 `0BFFA7AB5ED6E4C82680F9BFCB778318219675E6EC42AB4FEA1786380993CBBD`. A recursive filename search found no `*Splatmap*json` anywhere in the actual `C:/Users/tarik.bergstrom/Perforce/AGP-Content` root, including its runtime exports and Unreal project. Thus this scene's observed JSON currently lives only in AGP's ignored runtime content; its provenance/synchronization should be preserved explicitly later.

The entire JSON has one master material and three actors:

| Record | Exported content | Source |
| --- | --- | --- |
| `MM_Landscape` | Domain 0, shading model 1, blend mode 0, two sided false. Only parameter is scalar `DEBUG=0`. | [JSON lines 2–24](../../Content/ExportedScenes/lvl_splatmap_testing/Lvl_Splatmap_Testing_Level.json#L2) |
| `DirectionalLight` | Directional light component, intensity 20. | [JSON line 28](../../Content/ExportedScenes/lvl_splatmap_testing/Lvl_Splatmap_Testing_Level.json#L28) |
| `Landscape` | Native Landscape archetype, actor scale `(100,100,100)`, translation `(-2800,-2800,0)`. Only an identity `RootComponent0` of TypeID 0. No Landscape components, mesh, layer allocations, textures or material instance assignment. | [JSON line 84](../../Content/ExportedScenes/lvl_splatmap_testing/Lvl_Splatmap_Testing_Level.json#L84) |
| `SM_Landscape_Test` | Ordinary static mesh actor, identity rotation/scale, translation `(0,0,-30)`, TypeID 1 mesh component. Mesh reference `/Game/Meshes/Landscape/Test/SM_Landscape_Test.SM_Landscape_Test`. | [JSON line 133](../../Content/ExportedScenes/lvl_splatmap_testing/Lvl_Splatmap_Testing_Level.json#L133) |
| `MI_Landscape_SplatMapTesting` | Static mesh material with parent `MM_Landscape`; only parameter scalar `DEBUG=1`. | [JSON line 183](../../Content/ExportedScenes/lvl_splatmap_testing/Lvl_Splatmap_Testing_Level.json#L183) |

There are **no texture parameters, texture references, splat mask files/channels, painted weights, layer names, mask transform, noise inputs or surface tiling parameters in this JSON**. Master material naming and `DEBUG` do not encode the material graph. The native Landscape actor transform alone cannot establish weightmap coverage or layer packing. [Source: complete current JSON](../../Content/ExportedScenes/lvl_splatmap_testing/Lvl_Splatmap_Testing_Level.json).

The mesh import path in [JSON line 180](../../Content/ExportedScenes/lvl_splatmap_testing/Lvl_Splatmap_Testing_Level.json#L180) points to another author's Desktop. The corresponding local runtime asset **does** exist at [content-owned FBX](C:/Users/tarik.bergstrom/Perforce/AGP-Content/Content/Meshes/Landscapes/Test/SM_Landscape_Test.fbx), 85,632 bytes, but AGP's `Content/Meshes/Landscapes/Test/` is absent. A recursive AGP mesh filename search found `SM_Landscape_Test_01.fbx` under `OrientationTesting`, a different filename. No runtime loading was attempted; this is a deployment gap observation, not a rendering result.

## Why the exporter omits the splat setup

All source references below refer to the actual content checkout's [plugin source directory](C:/Users/tarik.bergstrom/Perforce/AGP-Content/Unreal/P5G4_KnightHeart/Plugins/TGAUnrealPlugin/Source/TGAUnrealPlugin/Private/).

- [TGALevelSerializer.cpp, lines 33–47](C:/Users/tarik.bergstrom/Perforce/AGP-Content/Unreal/P5G4_KnightHeart/Plugins/TGAUnrealPlugin/Source/TGAUnrealPlugin/Private/TGALevelSerializer.cpp#L33) registers scene, static/skeletal mesh, light, collision, spring arm and camera handlers. It has no Landscape handler. Component dispatch uses exact class lookup or the custom serialization interface at lines 440 and 703. This explains the native Landscape's surviving root metadata without its painted component data.
- [SerializeMaterial, line 211](C:/Users/tarik.bergstrom/Perforce/AGP-Content/Unreal/P5G4_KnightHeart/Plugins/TGAUnrealPlugin/Source/TGAUnrealPlugin/Private/TGALevelSerializer.cpp#L211) exports material identity, master summary and enumerated scalar/vector/texture/static-switch **parameters** (enumeration at lines 274, 289, 304, 316). It does not serialize expression nodes or graph links. A hardwired texture sample is not exported by this parameter enumeration.
- Texture parameters, if exposed, carry `Name`, Unreal `Path`, and `AssetImportPath` at [lines 381–397](C:/Users/tarik.bergstrom/Perforce/AGP-Content/Unreal/P5G4_KnightHeart/Plugins/TGAUnrealPlugin/Source/TGAUnrealPlugin/Private/TGALevelSerializer.cpp#L381). This writes references, not image pixels or Landscape weightmap extraction.
- The static mesh path exports mesh name/path/import path and material parameter records through [HandleMeshComponent, line 715](C:/Users/tarik.bergstrom/Perforce/AGP-Content/Unreal/P5G4_KnightHeart/Plugins/TGAUnrealPlugin/Source/TGAUnrealPlugin/Private/TGALevelSerializer.cpp#L715). It can already identify the converted terrain mesh; it does not create that FBX or bake a mask.
- [TGAUnrealPlugin.cpp, lines 178–208](C:/Users/tarik.bergstrom/Perforce/AGP-Content/Unreal/P5G4_KnightHeart/Plugins/TGAUnrealPlugin/Source/TGAUnrealPlugin/Private/TGAUnrealPlugin.cpp#L178) gathers base materials from mesh components and serializes their summaries. The export command calls navmesh FBX export and scene JSON export at lines 141–142; the FBX routine at line 334 reads Recast navigation, not visual terrain.
- [ExportAssets.bat](C:/Users/tarik.bergstrom/Perforce/AGP-Content/ExportAssets.bat) copies existing FBX source files and supported surface PNG filename patterns, then converts them to DDS. It neither extracts native Landscape weights nor transports arbitrary mask/noise names by default. `texconv.exe` **is present** in this actual content root, correcting the earlier device-specific planning observation of its absence.

AGP's importer consumes named material parameters, with texture lookup names derived from the import filename stem: [UnrealSceneImporter.cpp, line 237](../../Source/Engine/GameFramework/UnrealSceneImporter/UnrealSceneImporter.cpp#L237), [instance parsing, line 389](../../Source/Engine/GameFramework/UnrealSceneImporter/UnrealSceneImporter.cpp#L389). It cannot reconstruct missing graph nodes or painted weights from the observed JSON. A transport contract/bake must be established from live Unreal evidence before implementation.

## Preserved local state and limits

AGP branch was `splatmap-blending`, HEAD `517a6d7137f6fee2741d0a109b46126cf792062f`, origin `https://github.com/Troreen/AGP.git`. Its existing tracked modification is `Bin/Settings/ApplicationSettings.json`: current initial scene changed from shader testing to museum, so it does not currently select the splatmap scene. [Settings](../../Bin/Settings/ApplicationSettings.json). No edit was made.

AGP-Content is on `main...origin/main`, origin `https://github.com/Troreen/AGP-Content.git`. Relevant pre-existing changes include modified `MM_Landscape.uasset` and museum map; untracked `MI_Landscape_SplatMapTesting.uasset`, `Lvl_Splatmap_Testing.umap` and sharedassets, Unreal landscape test mesh folder, runtime `Content/Meshes/Landscapes/Test/`, and `T_Noise01.png/.dds`. Other animation/mesh/map edits and deletions were present and preserved. Exporter source did not appear in Git's modified/untracked list. These status observations establish unsaved repository scope, not the live graph's behavior.

No Perforce reconciliation/submission, content copying, scene re-export, shader/build or runtime verification was performed. Questions remaining for editor evidence: exact connected layer textures, native paint layer names/allocations, DEBUG behavior on converted static mesh, actual mapping/noise formulas, and a reproducible image/mask export route.
