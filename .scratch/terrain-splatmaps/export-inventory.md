# Local scene export inventory for splat blending

Inspected 2026-10-19. This is static research for required material blending with noise applied to weights, and optional material-detail height blending. It does not change the engine or establish a terrain-heightmap requirement.

## Method and scope

Used Python 3.14 standard-library `json`, `pathlib`, `collections`, `re` and `struct` through `py -3` to parse **all 12 JSON files** recursively under `Content/ExportedScenes`, containing **2,381 actor records** including both copies of the animation scene. Traversed actors, components, material instances, master-material records and texture-valued parameters. Correlated mesh/texture asset stems against files under `Content`; read selected DDS/PNG header dimensions without decoding pixels. Counts include duplicate exported records, not unique Unreal objects.

The Windows `python` alias was unavailable; the `py -3` launcher successfully used `C:/Program Files/Python314/python.exe`. No game, Unreal editor, renderer, importer or FBX geometry inspection was run. No screenshots or visual material claims are made.

## Complete scene census

Paths below are relative to `Content/ExportedScenes`.

| JSON | Actors | Master materials | Actual `Landscape` actors | Components whose mesh name contains `Landscape` |
| --- | ---: | ---: | ---: | ---: |
| `ChestMaterials_Level.json` | 5 | 0 | 0 | 0 |
| `lvl_01_diorama/Lvl_01_Diorama_Level.json` | 192 | 14 | 1 | 1 |
| `lvl_blistering_tundra/Lvl_Blistering_Tundra_Level.json` | 918 | 6 | 4 | 0 |
| `lvl_blockout/Lvl_Blockout_Level.json` | 44 | 3 | 1 | 1 |
| `Lvl_Blockout_Level.json` | 21 | 0 | 1 | 0 |
| `lvl_component_test/Lvl_Component_Test_Level.json` | 3 | 2 | 0 | 0 |
| `lvl_forsaken_castle/Lvl_Forsaken_Castle_Level.json` | 813 | 7 | 2 | 0 |
| `lvl_shader_testing_tarik/Lvl_Shader_Testing_Tarik_Level.json` | 4 | 3 | 0 | 0 |
| `lvl_topdown/Lvl_TopDown_Level.json` | 341 | 4 | 2 | 0 |
| `lvl_verify_anim/lvl_verify_anim/Lvl_Verify_Anim_Level.json` | 12 | 3 | 0 | 0 |
| `lvl_verify_anim/Lvl_Verify_Anim_Level.json` | 12 | 3 | 0 | 0 |
| `TestExportMap_Level.json` | 16 | 0 | 0 | 0 |

All **11 actual Landscape actor records** contain transforms and a single `RootComponent0` with `TypeID: 0`; none carries a mesh, material list, layer list, heightmap or weightmap reference. For a concrete example, see the [diorama Landscape record](../../Content/ExportedScenes/lvl_01_diorama/Lvl_01_Diorama_Level.json#L768). This establishes the contents of these checked-in exports, not the capabilities of another exporter version.

## Concrete mesh/material correlations

### Diorama landscape-named static mesh

[Actor `SM_EN_Landscape_01`](../../Content/ExportedScenes/lvl_01_diorama/Lvl_01_Diorama_Level.json#L9499) is a `StaticMeshActor`, separate from that scene's `Landscape` actor. Its `StaticMeshComponent0` has `TypeID: 1`, `Mesh: SM_EN_Landscape_01`, content path `/Game/Meshes/Props/Diorama/SM_EN_Landscape_01.SM_EN_Landscape_01`, and a workstation-specific FBX import path ending in `Meshes/Props/Diarama/SM_EN_Landscape_01.fbx`.

The corresponding local file exists at [Content/Meshes/Props/Diarama/SM_EN_Landscape_01.fbx](../../Content/Meshes/Props/Diarama/SM_EN_Landscape_01.fbx), 127,536 bytes. Actor rotation is nonidentity. No inference about UVs, topology, vertex colors or rendered terrain is justified by its name or file presence.

Its only material entry is `MM_Snow`, `Parent: Self`, `Parameters: []`. The scene's [master `MM_Snow`](../../Content/ExportedScenes/lvl_01_diorama/Lvl_01_Diorama_Level.json#L409) separately declares:

| Parameter | Type | Exported value |
| --- | --- | --- |
| `AlbedoMap` | 3, texture-valued object | `T_EN_Snow_01_C` |
| `ORMMap` | 3, texture-valued object | `T_EN_Snow_01_ORM` |
| `NormalMap` | 3, texture-valued object | `T_EN_Snow_01_N` |
| `Tiling` | 0, numeric | approximately 0.1 |

No local file under `Content` has an exact matching stem for those three snow texture names. Possible renamed counterparts exist: `T_EN_Snow_01_BaseColor` PNG/DDS, `T_EN_Snow_01_Normal.png`, and `T_EN_Snow_01_OcclusionRoughnessMetallic` PNG/DDS. Their headers report 1024 by 1024. This is a **name mismatch**, not proof that the importer resolves them or that their pixels are equivalent. The normal and ORM PNGs have 16-bit RGBA headers; that bit depth does not turn them into terrain heightmaps.

### Orientation test landscape-named static mesh

[Actor `SM_Landscape_Test_01`](../../Content/ExportedScenes/lvl_blockout/Lvl_Blockout_Level.json#L2406) is another `StaticMeshActor` with `TypeID: 1`, mesh `SM_Landscape_Test_01`, material `M_Blue`, `Parent: Self`, and empty instance parameters. Its JSON content path is `/Game/Meshes/Orientation_Testing/SM_Landscape_Test_01.SM_Landscape_Test_01`; its author import path ends in `Meshes/OrientationTesting/SM_Landscape_Test_01.fbx`.

The local [Content/Meshes/OrientationTesting/SM_Landscape_Test_01.fbx](../../Content/Meshes/OrientationTesting/SM_Landscape_Test_01.fbx) exists, 422,400 bytes. The actor translation recorded in the matrix is `(2700, 3130, 200)`. A separate `Landscape` root-only actor follows in the scene. There is no splat/weight/height texture parameter on this static mesh record.

The local [Lvl_01_Diorama_Landscape_o1.fbx](../../Content/Meshes/Props/Diarama/Lvl_01_Diorama_Landscape_o1.fbx), 451,552 bytes, also exists. Its stem occurs in none of the 12 scene JSON texts. Its geometry and intended scene relationship remain unverified.

## Noise-related material evidence

The exports already carry scalar controls named `Noise intensity` and `Noise Size`, but **their values do not encode the shader graph or identify what noise modifies**.

- Diorama [master `MM_Blockout`](../../Content/ExportedScenes/lvl_01_diorama/Lvl_01_Diorama_Level.json#L300) includes `Snow Tiling` approximately 0.1, `Noise intensity` approximately 0.7 and `Noise Size` 4, plus `AlbedoMap`, `NormalMap`, `ORMMap` texture references. Instance `M_Trimsheet_Wood_Snow` on actor/mesh `SM_Prop_Outhouse` uses parent `MM_Blockout` and the same noise controls.
- Diorama [instance `MI_Cliff`](../../Content/ExportedScenes/lvl_01_diorama/Lvl_01_Diorama_Level.json#L9715), on actor `SM_Env_Cliff_6x6x16`, mesh `SM_Env_Cliff_6x6x4`, also uses `MM_Blockout`. It exports `Noise intensity` approximately 0.865775 and `Noise Size` approximately 1.876324, plus `T_4M_Cliff_A_BaseColor`, `T_4M_Cliff_A_Normal`, and `T_4M_Cliff_A_OcclusionRoughnessMetallic` references. Exact local PNG/DDS stem matches exist for all three; selected headers report 512 by 512. Their names suggest normal material textures, not evidence of a splat control mask.
- [Shader-test master `MM_Standard_WithSnow`](../../Content/ExportedScenes/lvl_shader_testing_tarik/Lvl_Shader_Testing_Tarik_Level.json#L4) exports noise controls approximately 0.7 and 4, `Snow Tiling` approximately 0.1, plus brick albedo/normal references. Master records do not serialize a material graph.
- In the tundra scene, actor `SM_Bridge_Wood_A2`, mesh `SM_Bridge_Wood_A`, carries material `MI_MedievalBricks_Snow`, parent `MM_Standard_WithSnow`, and those noise controls. In the castle scene, actor/mesh `SM_Castle_Wall_A_4M` also carries `MI_MedievalBricks_Snow`. These are ordinary static-mesh material examples; their snow/noise names do not establish weight-based landscape blending.

Both noise parameter names occur in five scene files: diorama, tundra, castle, shader test, and topdown. The shader-test JSON has them only in the master record, so their presence alone does not imply an applied instance override.

## Splat/weight/detail-height evidence and limits

No occurrence of the literal tokens `splat`, `weightmap` or `heightmap` exists in any of the 12 JSON texts. A traversal found 54 distinct `(parameter name, texture name, Unreal path)` combinations; none of the texture parameter names identifies splat, weight, height, noise or layer maps. Existing texture names include ordinary albedo, normal, ORM, opacity, HDRI and legacy Phong slots.

`OpacityMaskMap`, `OpacityMaskMapWeight` and various `...MapWeight` values occur on legacy player materials. These are not proof of terrain blend weights. Likewise `Blend Offset` occurs on mannequin materials, and `UseBlendWorldPosition` on HDRI material records; neither establishes the requested blending mechanism.

The filename scan under `Content` found noise-named DDS files in `Textures/BloodTextures`, but none was referenced as a noise texture by the scanned JSON texture parameters. Neither file names nor this negative parameter-name search can rule out masks packed under generic names, vertex-color-driven blending, or manually configured assets elsewhere. Pixel contents and the Unreal source material graph were not inspected.

## Planning implications — recommendations

1. Keep the first required capability about material blending on existing geometry. The available examples do not establish a requirement to generate terrain geometry from a heightmap.
2. Obtain or author one representative splat mask with an explicit channel-to-material table and coordinate mapping. The checked-in JSONs do not supply this contract.
3. Specify noise as modifying the blend weights before material mixing, with scale, strength, normalization and zero-total fallback behavior. Do not infer that the existing `Noise intensity`/`Noise Size` values already implement that behavior.
4. Treat detail-height blending as a separately selectable material enhancement. No exported height parameter or documented height channel was found in this inventory; ORM and normal assets should not be reinterpreted as heights.
5. Decide how new splat/noise/detail-height references enter the import pipeline, and how the current snow texture name mismatches resolve, after the parent engine/importer investigation. Exported parameter values and file presence alone do not demonstrate that AGP binds or renders them.

Only this research note was created for this inventory task. No staging, commits or runtime verification occurred.
