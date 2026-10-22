# Splatmap assets and geometry investigation

Observed 2026-10-22, research only. This track inspected file bytes and existing
source; it made no editor queries, conversions, exports, source changes, content
copies, version-control changes or runtime claims. Root owns live Unreal material
and import-setting evidence. The intentional RGB/alpha-zero debug material is
not the production surface appearance.

## Roots, ownership and evidence method

- Content root: `C:/Users/tarik.bergstrom/Perforce/AGP-Content`.
- Unreal root: `<content-root>/Unreal/P5G4_KnightHeart`; `/Game/` corresponds to
  its `Content/` directory.
- Runtime asset root: `<content-root>/Content/`, separately deployed to AGP's
  ignored `Content/`. No deployment occurred in this pass.
- Instructions read: both root `AGENTS.md`, content `AgentConfig/GlobalRules.md`,
  `Docs/ProjectVersionControl.md`, content `README.md`, investigation plan and
  `current-export-research.md`.
- Read-only content Git origin is `https://github.com/Troreen/AGP-Content.git`.
  Git status could not be refreshed by this track: `git -C <content-root>
  rev-parse --show-toplevel` reported permission denied, while status from a
  tool workdir reported it was outside a worktree. Do not infer a clean checkout.
  Root's initial status record remains the baseline. File reads worked normally.

Binary FBX was parsed in memory with Blender 5.0's installed standalone
`fbx2json.py` parser (`parse`, not its file-writing conversion command), using
Python `-B` and no output file. Names below are FBX tree paths, so binary
observations do not have text line numbers. PNG metadata/pixels and DDS header
fields were inspected directly. SHA256 comparisons establish byte identity,
not successful runtime rendering.

## Current test terrain has a matching local FBX

The current scene JSON references `/Game/Meshes/Landscape/Test/SM_Landscape_Test`
and an author-machine import path ending
`Content/Meshes/Landscapes/Test/SM_Landscape_Test.fbx`.
[JSON lines 178-180](../../Content/ExportedScenes/lvl_splatmap_testing/Lvl_Splatmap_Testing_Level.json:178).
The singular Unreal package folder and plural runtime FBX folder are different
names; a filename-stem lookup can still resolve them, but paths must not be
guessed by substituting `/Game/`.

The local matching file is
`<content-root>/Content/Meshes/Landscapes/Test/SM_Landscape_Test.fbx`:

| Evidence | Observation |
| --- | --- |
| File size | 85,632 bytes |
| SHA256 | `0ea2e05ef3143bf1510dd477ad2f0c8f50f0af158db40bdded265070e1a75eed` |
| MD5 | `18ba5d2bc52b9808e6c2f7f522e18417` |
| Unreal package import-data `FileMD5` | **Exact same MD5**, in `/Game/Meshes/Landscape/Test/SM_Landscape_Test` saved package |
| FBX version | 7300 |
| FBX original and last-saved application | Epic Games, Unreal Engine, `5.7.4-51494982+++UE5+Release-5.7` |
| FBX creation string | `2026-10-22 14:29:14:659` |
| Source document URL | Author's Desktop path, same tail as JSON |
| FBX embedded geometry | One `Landscape` mesh, 4,096 control points, 18,816 polygon indices / 6,272 triangles |
| FBX embedded material | `M_Landscape`; no texture/video records in this file |

The MD5 link is stronger provenance than a matching filename. The file itself
records an Unreal export. It does not demonstrate an intervening Blender/Maya
conversion or prove that the historical museum workflow used the same route.
The embedded FBX material does not carry this scene's three painted layer masks.

There is no `SM_Landscape_Test.fbx` beside its Unreal `.uasset` in
`Unreal/.../Content/Meshes/Landscape/Test/`; that directory contains only the
159,967-byte package. Thus the existing batch copy from Unreal source FBXs
would not recreate this particular runtime FBX if the runtime copy were lost.
AGP's current `Content/Meshes/Landscapes/Test/` is absent. This remains a concrete
deployment gap; no load failure was induced or observed.

## Exact coordinates, units and UVs

FBX binary paths `/GlobalSettings/Properties70`, `/Objects/Geometry` and
`/Objects/Model/Properties70` provide:

| Property | Value |
| --- | --- |
| Up axis/sign | 2 / +1 (Z up) |
| Front axis/sign | 1 / -1 |
| Coord axis/sign | 0 / +1 |
| UnitScaleFactor / OriginalUnitScaleFactor | 1 / 1 |
| Local vertex X | 0 to 56 |
| Local vertex Y | -56 to 0 |
| Local vertex Z | 0 to 0.2421875 |
| Model local translation | (-2800, +2800, 0) |
| Model local scaling | (100, 100, 100) |
| TextureUVs, `LayerElementUV[0]` | U and V both 0 to 56; `ByVertice`, `Direct` |
| WeightmapUVs, `LayerElementBumpUV[0]` | U and V both 0 to 56/57 = 0.9824561403508771; `ByVertice`, `Direct` |

Full transform follow-up: the file has only one Model, Landscape
ID2297447801744, connected directly to FBX root0. Its properties override only
LclTranslation, LclScaling, ScalingMax(0,0,0) and DefaultAttributeIndex0. It has
no explicit LclRotation, PreRotation, PostRotation, pivot, rotation/scaling
offset or geometric-transform overrides. The Model's `FbxNode` property
template supplies LclRotation/PreRotation/PostRotation/RotationPivot/
RotationOffset/ScalingPivot/ScalingOffset/GeometricTranslation/GeometricRotation
all(0,0,0), GeometricScaling(1,1,1), RotationOrder0. There is no hidden rotated
parent node. Thus the saved FBX Model transform itself contains no quarter turn.

All 4,096 points satisfy `TextureUVs = (localPosition.x,
-localPosition.y)` exactly. All WeightmapUV coordinates equal `TextureUVs/57`
within floating-point rounding (`1.11e-16` maximum residual). Both channels are
declared in FBX Layer 0, but the second is specifically **BumpUV**, not evidence
that the proprietary importer exposes it as ordinary UV1.

Applying the FBX model transform gives X/Y extents -2800 to +2800 and Z 0 to
24.21875 in its centimetre scale. Root's live mesh bounds independently match
these extents. The JSON actor then adds Unreal world Z=-30 at unit scale
([JSON lines 134-152](../../Content/ExportedScenes/lvl_splatmap_testing/Lvl_Splatmap_Testing_Level.json:134)).
These observations support a 56m square and an 8m surface tile producing seven
repeats across the test mesh. They do not establish AGP's loaded mesh axis
conversion, node-transform flattening, tangent handedness or final UV V flip.

In particular, **raw FBX Y is not automatically Unreal Y**. Installed UE5.7
primary source shows handedness conversion: legacy
`Engine/Source/Editor/UnrealEd/Private/Fbx/FbxUtilsImport.cpp:63-69` negates Y
in `ConvertPos`; legacy `FbxStaticMeshImport.cpp:758` writes UV `(U,1-V)`.
Interchange's
`Engine/Plugins/Interchange/Runtime/Source/Parsers/Fbx/Private/FbxMesh.cpp:943`
also writes `(U,1-V)`, while its `FbxConvert.h:134` provides the position
conversion. The alternate Ufbx path targets left-handed Z-up with Y mirror at
`Private/Ufbx/UfbxParser.cpp:257-258`, and
`Private/Ufbx/UfbxConvert.h:48-50` likewise flips UV V. These are source paths
under `C:/Program Files/Epic Games/UE_5.7/`. The saved mesh package contains
InterchangeAssetImportData, but its exact pipeline options and loaded vertex
samples were not measured. Consequently the raw formula `UV=(x,-y)` is only
an FBX-local observation and **does not prove a world V reversal in Unreal**.

For the native Landscape, the semantics track establishes grid UVs. With actor
translation(-2800,-2800) and XY scale100, native surface UV0/8 is
`(UnrealWorldXY + (2800,2800))/800`. Absolute `UnrealWorldXY/800` would remove
3.5 repeats of phase on each axis (half a repeat modulo wrapping). Retaining
8m frequency alone therefore does not guarantee native-reference texture
alignment. This phase question is separate from splat-mask registration and
from the converted static mesh's imported UV convention.

AGP's [AssetLoadingHelper.cpp:96](../../Source/Engine/GameFramework/AssetHandling/AssetLoadingHelper.cpp:96)
copies the imported UV0 and UV1 unchanged; its input comes from the binary
TGAFBX importer. Only the importer's public headers/libraries are in this
checkout. Therefore no source evidence establishes what that library does
before handing these arrays to AGP. A read-only imported-buffer dump or a
deliberately asymmetric rendering check would close this gap, with permission
if it requires a new experiment.

The raw FBX normals point almost entirely +Z (minimum Z 0.99776759). Its first
flat tangent is approximately (-1,0,-0.00392154), and first binormal is
(0,-0.99999231,0.00392154). This argues against assuming tangent orientation
from the position axis names alone; the runtime-contract track owns the
projected-normal decision.

The `WeightmapUVs` range also must not be used blindly as the baked 512x512 mask
transform. It encodes a vertex-sample count denominator (57) in this FBX, whereas
the native landscape's world extent spans 56 quads. Root/semantics track must
establish the `render_weightmaps` pixel registration and image orientation.

### Follow-up: measured Unreal static-mesh coordinates

Root's read-only [live mesh sample](audit-live-mesh-uv.json) subsequently closed
the Unreal vertex/UV gap for this exact mesh, checking the first vertex instance
at each of all4,096 vertices. The fitted formulas in **Unreal mesh-local axes**
are:

```text
UV0 = ((2800 - localY) / 100, 1 - (localX + 2800) / 100)
UV1 = ((2800 - localY) / 5700, 1 - (localX + 2800) / 5700)
```

UV0 ranges U=0..56,V=-55..1; UV1 ranges U=0..0.9824561476707458,
V=0.01754385232925415..1. UV0 formula residual is1.0658e-14 for both channels;
UV1 residuals are2.928e-8 and4.183e-8. At vertex0, local position
(-2800,+2800,0) has UV0(0,1), UV1(0,1). At vertex4095,
(+2800,-2800,0) has UV0(56,-55), UV1(0.98245615,0.01754385).

Because the scene actor's XY transform is identity, the current Unreal static
mesh's surface UV0/8 is
`((2800-worldY)/800, 1/8-(worldX+2800)/800)`. It has seven8m repeats across
the56m span, with U toward-Y and V toward-X. It is measurably different from
native Landscape grid UV0/8 and the requested positive UnrealXY world projection.
The stored UV1 is also distinct from a fresh512x512 world-space mask mapping.

This is direct Unreal mesh evidence; it supersedes any guess based solely on
the raw FBX axes above. It still does not reveal AGP's proprietary TGAFBX
importer's outputs, renderer tangent frame or runtime mask alignment. Coverage
is the first vertex instance per vertex, not every per-corner instance or LOD.

### Follow-up: orientation cause traced to saved import settings

Root subsequently captured the persisted Interchange common-pipeline setting
`ImportOffsetRotation=(Pitch=0,Yaw=-90,Roll=0)` at
[audit-import-pipeline-assets.t3d:92](audit-import-pipeline-assets.t3d:92).
This is direct evidence for the mesh quarter turn, superseding an earlier
unconfirmed possibility of ForceFrontXAxis conversion. The semantics agent's
`uv-orientation-cause.md` owns the complete source derivation: native Landscape
exports grid UV(u,v) without the V flip used by the ordinary StaticMesh exporter,
then Interchange imports UV(u,1-v). The persisted yaw-90 rotates geometry while
the UV values stay attached to their corresponding vertices.

The raw/current samples make the mapping concrete:

| Control point / live vertex ID | Raw FBX xyz | Raw surface UV | Actual Unreal position | Actual Unreal UV0 |
| --- | --- | --- | --- | --- |
| 0 | (0,0,0) | (0,0) | (-2800,2800,0) | (0,1) |
| 1 | (1,0,0) | (1,0) | (-2800,2700,0) | (1,1) |
| 56 | (0,-7,0) | (0,7) | (-2100,2800,0) | (0,-6) |
| 4095 | (56,-56,0) | (56,56) | (2800,-2800,0) | (56,-55) |

All eight saved live position samples match
`UEPosition=(-2800-100*rawY,2800-100*rawX,100*rawZ)` exactly. The raw FBX
node has no rotation, so the quarter turn is introduced by the saved import
pipeline, not MM_Landscape's UV arithmetic. This explains the present Unreal
asset; reproducing its import offset in AGP remains a separate transport
decision because the JSON actor rotation is identity and TGAFBX need not read
Unreal's saved import-pipeline settings.

## Older similarly named terrain is a different asset

`Content/Meshes/OrientationTesting/SM_Landscape_Test_01.fbx` exists in both
content and AGP; its content-runtime and Unreal-source copies are byte-identical.
Its SHA256 is `ee1628bd2192abbbfea5d4ed8e5c592db72df8af23ba03be24c9caebb0edb284`
and size 422,400 bytes. It has 16,384 control points / 31,752 triangles, local
X=189..315, Y=-315..-189, Z=-33.046875..0, model translation
(-25230,+25200,0), model scale100, UV0=189..315, BumpUV0..126/127. Its creation
string is 2026-09-30; it also records Unreal 5.7.4 as original/last-saved app.
This is a 126m region with a different placement and silhouette, not a safe
fallback for the current test terrain. Content's blockout export references
this `_01` asset at `Lvl_Blockout_Level.json:2406,2451-2453`.

## Nine surface textures: source and deployed headers

All nine source PNGs exist at `Unreal/.../Content/Textures/`. All are 1024x1024,
8-bit RGB (`IHDR` color type2, no alpha), with no observed `sRGB`, `gAMA` or
`cHRM` chunks. These PNG bytes alone do not establish Unreal's imported sRGB or
normal-green setting; root captures those live properties.

Their runtime DDS files exist under **both** `<content-root>/Content/Textures/`
and AGP `Content/Textures/`; every pair is byte-identical. All have 11 mips.
DDS fields used: height offset12, width16, mip count28, pixel-format FourCC84,
DX10 DXGI format128.

| Runtime filename | Format observed | SHA256 |
| --- | --- | --- |
| `T_8M_Snow_Dunes_BaseColor.dds` | DX10 DXGI99 = BC7_UNORM_SRGB | `3d02c1ed4ba49ab09de5f388e30d5b4bec17bd130f5e7b0ce754761d23e616e1` |
| `T_8M_Snow_Dunes_OcclusionRoughnessMetallic.dds` | DX10 DXGI98 = BC7_UNORM | `e1e8fbcf613e06aee669d10676429ef64bc6a579d422cff255b9e7884065b0d2` |
| `T_8M_Snow_Dunes_Normal.dds` | Legacy FourCC BC5U = BC5_UNORM | `3b51d781b3ae050dd7510c3784ad7349cdb906b54d6830882c875b2733c1d898` |
| `T_8M_Snow_Rough_BaseColor.dds` | BC7_UNORM_SRGB | `3d02c1ed4ba49ab09de5f388e30d5b4bec17bd130f5e7b0ce754761d23e616e1` |
| `T_8M_Snow_Rough_OcclusionRoughnessMetallic.dds` | BC7_UNORM | `3155bc149dab9f01f98e7608862a1883a601390b046e9447419a4d872a4e2a90` |
| `T_8M_Snow_Rough_Normal.dds` | BC5_UNORM | `e4b35692002d8dca8e48ddcccddd43705babdd79c8d0018a89159bc0e5c6d9bc` |
| `T_8M_Snow_Cliff_BaseColor.dds` | BC7_UNORM_SRGB | `db9b193cfbcf373962f00815839ea6b537f3147a27a983e65c58529e4015d9a7` |
| `T_8M_Snow_Cliff_OcclusionRoughnessMetallic.dds` | BC7_UNORM | `bd59c89a31ff65a3dc5f14ea0c59931e03802585f60099fd3703b1e5700c92f1` |
| `T_8M_Snow_Cliff_Normal.dds` | BC5_UNORM | `26e73c23c2faa5fe0ad7dc17dfd07b3a7687b6213e7e2b84c6636d3821e2b840` |

BC7 files are each1,398,276 bytes; BC5 files each1,398,256 bytes. Three layers
therefore occupy about12MiB of compressed DDS payload including full mips,
before a mask/noise. This is file-size evidence, not measured VRAM allocation.

Important pixel findings: Dunes and Rough BaseColor source PNGs are identical,
uniform RGB(131,145,147), and their DDS hashes match each other too. Rough ORM
source is uniform RGB(255,210,0). Consequently a basecolor-only screenshot
cannot prove the two snow layers' distinct normal/roughness blending. The live
material audit confirms ORM pin meanings; filenames alone do not prove packing.
The batch conversion command has no `--invert-y`, so it does not explicitly flip
normal green. Actual Unreal imported `FlipGreenChannel` and runtime tangent
conventions must be compared separately.

## Noise candidates and conversion route

`T_Noise01.png` and `.dds` are present in content and AGP runtime roots, with
each corresponding file byte-identical. PNG:256x256 RGBA8, RGB channels identical,
range0..252, mean120.888626, alpha255 everywhere, no gamma/sRGB chunks. Visual
inspection shows smooth multi-scale grayscale noise. Opposite-edge mean absolute
byte differences are3.378906 along X and2.5625 along Y; this is an observation,
not proof of seamless filtering or suitability at an unspecified world scale.

Its DDS is256x256,9mips,DXGI99 **BC7_UNORM_SRGB**,87,556bytes. SHA256
`3808ed02be2c094823b713f34dd2ca03cdfde56155c38d636cf6473111a316b2`.
If used as numeric noise for weight perturbation, sRGB sampling changes its
distribution; this needs an intentional decision or a linear export. No
conversion was performed. There is no same-named source under Unreal Content
in the filename inventory, so its import/authoring history remains unknown.

Other existing runtime noise candidates in `Content/Textures/BloodTextures/`
are `MergedBloodNoiseChannels`, `MergedDistortNoiseChannels`,
`T_SkewTrypophobiaNoise`, `T_TrypophobiaNoise` (all1024x1024,11mips,legacy DXT1),
and two BC5U normal-noise files (`N_SkewTrypophobiaNoiseNormal`,
`T_TrypophobiaNoiseNormal`). Their AGP/content copies match. They have Unreal
packages under `Textures/ShaderTextures/T_M_Blood/`, but neither filename nor
presence makes them an intended terrain input. Root's connected graph audit
must establish whether any noise is currently connected.

`<content-root>/texconv.exe` is installed and its read-only `--help` reports
DirectXTex2026.5.8.1. Help confirms format selection `-f`, mip count `-m`,
`--ignore-srgb`, `--srgb-in`, `--srgb-out`, `--invert-y`, `--swizzle`, and output
directory `-o`; R8G8B8A8_UNORM,BC7_UNORM and BC4_UNORM are supported. This is
available capability, not a selected or tested mask command.

[ExportAssets.bat:12-14](C:/Users/tarik.bergstrom/Perforce/AGP-Content/ExportAssets.bat:12)
fixes source/destination/texconv paths relative to the script.
[Lines32,55](C:/Users/tarik.bergstrom/Perforce/AGP-Content/ExportAssets.bat:32)
copy existing FBXs and only named BaseColor/C/ORM/Normal PNG patterns. Conversion
uses `-y -f BC7_UNORM_SRGB -m 0` for BaseColor/C at72,
`BC7_UNORM -m 0` for ORM at88, `BC5_UNORM -m 0` for Normal at104, then deletes
each runtime PNG after success and DDS existence. It has no dedicated
splatmap/noise suffix or extraction step. Renaming a mask as BaseColor would
select the sRGB route; that would require explicit handling, not accidental reuse.

## Duplicate material paths and map references

Offline package strings establish these saved references, not active graph links:

- `/Game/Materials/MM_Landscape`:27,361bytes, references MF_LandscapeLayer and
  all nine snow textures. Live root evidence owns connectivity.
- `/Game/Materials/MI_Landscape`:8,312bytes, references the above master plus
  older `T_EN_Snow_01_C`, `_N`, `_N_ORM` packages. These could be stale parameter
  references; no active override conclusion follows from strings alone.
- `/Game/LandscapeMaterial/MM_Landscape` and `MI_Landscape`:1,313 and1,345bytes;
  both contain `ObjectRedirector` and reference the corresponding Materials
  package. Strong evidence of redirectors; root can confirm asset class in MCP.
- Debug instance saved package references `/Game/Materials/MM_Landscape`.
- Current saved test map references debug instance, three Snow LayerInfos and
  `/Game/Meshes/Landscape/Test/SM_Landscape_Test`.
- Museum saved map references `/Game/Materials/MM_Landscape` and corresponding
  three Snow LayerInfos. It does not establish which actor uses them.
- TopDown `Lvl_01_Diorama.umap` references `/Game/Materials/MI_Landscape`,
  `/Game/LandscapeMaterial/Snow2_LayerInfo`, and ordinary terrain-like static
  meshes such as `/Game/Meshes/Props/Diorama/SM_EN_Landscape_01`.

Content-owned exported JSON filename/text searches found no current splatmap
test export. AGP's current test JSON is therefore still a runtime-only artifact.
Its missing mask inputs are traced by `current-export-research.md`.

## Remaining evidence/owner decisions for integrated investigation

1. Root captures live imported texture sRGB, compression, normal-green flags,
   redirector class, and connected material/functions. No offline substitute
   can establish live unsaved values.
2. Runtime verification must establish imported mesh bounds, handedness,
   UV0 and tangent orientation. Existing AssetLoadingHelper does not flip UVs,
   but the proprietary importer before it may. Do not add a V flip from guesswork.
3. Register one asymmetric native painted feature against the baked mask,
   matching world bounds and pixel centres; distinguish extent56 quads from
  57 vertex samples and `WeightmapUVs`'57 denominator.
4. Ask owner whether current test map is authoritative MVP reference and how
   the museum production terrain/floaters should be included. Current FBX
   header answers this test source's provenance; it does not document every
   historical external conversion step.
5. Choose deliberate linear mask/noise formats and source ownership. Existing
   nine textures already deploy identically, but current mesh, JSON and Noise01
   authoring/deployment gaps need scoped later work, not broad synchronization.

No tests/builds/rendering were run. The evidence above is sufficient to avoid
guessing source asset identity, geometry scale, UV content or DDS formats;
it does not declare material parity or runtime alignment verified.
