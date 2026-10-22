# Splatmap runtime contract investigation

Observed 2026-10-22 on `splatmap-blending`, HEAD
`517a6d7137f6fee2741d0a109b46126cf792062f`. Research only. No source code,
settings, shaders, assets, builds, tests, scene loads, Git staging/commits or
Perforce operations performed. This agent owns only this note. All code-path
conclusions below are source observations or deductions; none is a rendered
result.

## Conclusions that affect the plan

1. Existing scalar/vector/texture transport is useful but cannot bind the nine
   current surface maps independently: every matching surface suffix goes to
   the same fixed texture slot, ignoring the exported parameter name.
2. The texture **register budget fits** a fixed three-layer material, even if
   all nine maps use custom registers. This does not resolve naming/binding,
   missing JSON inputs, shader implementation, packing or validation.
3. The material constant-data cap is **128 bytes**, with a reflection-offset
   hazard for HLSL padding. A few explicitly packed float4 rows fit; merely
   counting scalar sizes does not prove a layout works.
4. AGP already receives world position and can write final world normals.
   Unreal XY is AGP ZX. Reusing the mesh tangent frame after changing texture
   coordinates is insufficient to establish world-projected normal alignment.
5. Current runtime DDS formats have the expected data/color distinction:
   base color BC7 sRGB, ORM BC7 linear, normals BC5 unsigned linear. The newly
   observed Unreal ORM `sRGB=True` is a **separate discrepancy**, despite correct
   R/G/B graph wiring; do not call Unreal/runtime ORM equivalence verified.

## End-to-end current path

| Stage | Actual behavior | Primary source |
| --- | --- | --- |
| Scene JSON | `MM_Landscape` has scalar DEBUG=0; static mesh instance has DEBUG=1. No mask, nine surface maps, noise or world-to-mask transform. Native Landscape record has only a scene root. | [Current JSON](../../Content/ExportedScenes/lvl_splatmap_testing/Lvl_Splatmap_Testing_Level.json#L2), [instance](../../Content/ExportedScenes/lvl_splatmap_testing/Lvl_Splatmap_Testing_Level.json#L178) |
| Parameter parser | Type 0 becomes float; Type 1 becomes four-component vector, despite the local enum name Vector3f; Type 3 texture selection becomes the **stem of AssetImportPath**. Other types are skipped. Master and instance paths duplicate this logic. | [UnrealSceneImporter.cpp:237](../../Source/Engine/GameFramework/UnrealSceneImporter/UnrealSceneImporter.cpp#L237), [instance:402](../../Source/Engine/GameFramework/UnrealSceneImporter/UnrealSceneImporter.cpp#L402), [enum](../../Source/Engine/GameFramework/UnrealSceneImporter/UnrealSceneStructs.h#L30) |
| Intermediate scene | Texture values become strings containing that import stem; the original texture object's Name/Path do not select resources. | [UnrealSceneImporter.cpp:638](../../Source/Engine/GameFramework/UnrealSceneImporter/UnrealSceneImporter.cpp#L638), [instance:721](../../Source/Engine/GameFramework/UnrealSceneImporter/UnrealSceneImporter.cpp#L721) |
| Asset lookup | Registry indexes DDS under Content, creates lower-case filename-stem aliases and loads textures lazily. Duplicate stems overwrite the alias without a duplicate error here. Texture directories and Unreal package paths have already been discarded by the scene parser. | [AssetRegistry.cpp:25](../../Source/Engine/GameFramework/AssetHandling/AssetRegistry.cpp#L25), [indexing:68](../../Source/Engine/GameFramework/AssetHandling/AssetRegistry.cpp#L68), [lookup](../../Source/Engine/GameFramework/AssetHandling/AssetRegistry.h#L33) |
| Master material | Creates an AGP material from summary properties; looks for `<master name>.hlsli` in configured shader folder, uses missing-shader fallback if absent, applies master values to an instance, pins it by master name. | [SceneWorldBuilder.cpp:61](../../Source/Engine/GameFramework/Scenes/SceneWorldBuilder.cpp#L61), [shader selection:103](../../Source/Engine/GameFramework/Scenes/SceneWorldBuilder.cpp#L103) |
| Mesh instance | Reuses an already indexed/pinned material by name first. Otherwise creates from its parent, applies parameters and pins the instance. Same-named instances are therefore shared, not an arbitrary per-actor parameter transport seam. | [SceneWorldBuilder.cpp:257](../../Source/Engine/GameFramework/Scenes/SceneWorldBuilder.cpp#L257) |
| Scalar/vector binding | Replaces spaces with underscores, adds MB_ if absent, looks up reflected member name, requires matching type and byte size. | [SceneWorldBuilder.cpp:285](../../Source/Engine/GameFramework/Scenes/SceneWorldBuilder.cpp#L285), [Material.h:171](../../Source/Engine/GraphicsEngine/Materials/Material.h#L171) |
| Texture binding | `_BaseColor` -> t0, `_Normal` -> t1, `_OcclusionRoughnessMetallic` -> t2, `_Emissive` -> t3. Other stems select a reflected shader resource by that stem, case-insensitively. The JSON parameter Name has no binding priority. | [SceneWorldBuilder.cpp:301](../../Source/Engine/GameFramework/Scenes/SceneWorldBuilder.cpp#L301), [Material.cpp:197](../../Source/Engine/GraphicsEngine/Materials/Material.cpp#L197) |
| Draw | Refreshes inherited parameter data when dirty, uploads 128 bytes at b4, gathers all 16 material textures and binds t0..t15 for vertex and pixel stages. Missing custom textures are null; first four have engine defaults. | [GraphicsEngine.cpp:1511](../../Source/Engine/GraphicsEngine/GraphicsEngine.cpp#L1511), [binding implementation](../../Source/Engine/GraphicsEngine/RHI/GraphicsCommandList.cpp#L346) |

Current asset-name mismatch: JSON expects `SM_Landscape_Test`; AGP's observed
runtime file is `SM_Landscape_Test_01.fbx` in OrientationTesting, while the exact
FBX exists only in AGP-Content's `Content/Meshes/Landscapes/Test/`.
`ContentPath` and an absolute `AssetImportPath` in JSON do not make the registry
load that external FBX: scene mesh selection uses the exported mesh name.
Missing mesh falls back to a cube and can obscure the actual loading problem.
See [current export research](current-export-research.md),
[mesh conversion](../../Source/Engine/GameFramework/UnrealSceneImporter/UnrealSceneImporter.cpp#L710)
and [fallback](../../Source/Engine/GameFramework/Scenes/SceneWorldBuilder.cpp#L343).

## Binding/reflection limits, with the arithmetic checked

The engine cap is t0..t15, not the hardware pixel-stage resource limit. It sets
four defaults: white, flat normal, ORM `(255,128,0,255)`, and zero emission.
The entry-point shader declares/samples t0..t3 before the custom material hook.
See [MaterialInterface.h:65](../../Source/Engine/GraphicsEngine/Materials/MaterialInterface.h#L65),
[defaults](../../Source/Engine/GraphicsEngine/GraphicsEngine.cpp#L112),
[GBuffer_PS.hlsl:6](../../Source/Engine/GraphicsEngine/Shaders/Material/GBuffer_PS.hlsl#L6).

| Candidate allocation, not implemented | Custom slots | Including t0..t3 | Verdict |
| --- | --- | --- | --- |
| All nine surface maps custom t4..t12; mask t13; noise t14 | 11 of 12 | 15 of 16 | Fits; one register remains. Requires a binding route that bypasses the existing surface-suffix collapse. |
| One surface in standard t0..t2; other six maps custom t4..t9; mask t10; noise t11 | 8 of 12 | 12 of 16 | Fits; six custom surface bindings still need distinct supported names or a binder change. |
| All nine current surface names added as ordinary JSON texture parameters | Not a distinct nine-map allocation | Same t0/t1/t2 repeatedly assigned | Does not work: later values overwrite earlier values by suffix. |

Thus `9 + mask + noise = 11 < 16` alone was incomplete reasoning. The conservative
all-custom plan also retains four standard registers, and actual transport must
be fixed or adapted. It is nevertheless **15 <= 16**, so this investigation does
not justify increasing the engine cap or introducing texture arrays.

Resource reflection admits Texture resource type 2 with BindPoint < 16 and
stores resource name -> starting slot. It merges vertex then pixel resources;
same resource name at different registers warns and retains the first. It does
not establish nine surface assignments from parameter labels. Reflection records
array BindCount, but slot registration does not expand that count, so arrays of
resources are not a drop-in shortcut. Use unique live sampled Texture2D resources
for a minimal candidate. Optimized-out resources cannot be assumed bindable.
Sources: [CreateMaterial](../../Source/Engine/GraphicsEngine/GraphicsEngine.cpp#L298),
[CreateMaterialTextureSlots](../../Source/Engine/GraphicsEngine/GraphicsEngine.cpp#L476),
[resource reflection](../../Source/Engine/GraphicsEngine/RHI/RHIShaderReflectionInfo.cpp#L150).

The current GBuffer path performs four standard source samples, then calls the
hook. A three-surface/noise proposal would express another 9+1+1 logical reads
if it resamples all maps. Compiler dead-code removal may eliminate overwritten
inputs, but normal and emission debug branches can retain standard samples.
No shader has been compiled for this proposal, so instruction count, fetch count
after optimization, GPU time, occupancy and frame-rate cost remain unmeasured.

### Constant bytes and a concrete hazard

`MATERIAL_BUFFER_SIZE` is 128 bytes (eight 16-byte rows). Instance writes check
the buffer bounds. Creation copies reflected default values into a 128-byte
array without an explicit total reflected-buffer-size check in that loop.
Keep the complete reflected layout within 128 bytes, including padding; do not
interpret the comment mentioning the 64 kB hardware bound as available storage.
Sources: [cap](../../Source/Engine/GraphicsEngine/Materials/MaterialInterface.h#L65),
[write checks](../../Source/Engine/GraphicsEngine/Materials/Material.cpp#L346),
[creation](../../Source/Engine/GraphicsEngine/GraphicsEngine.cpp#L304).

More critically, reflection currently uses an accumulated `Rows * Columns * 4`
offset, rather than the HLSL variable's `StartOffset`. It also returns early for
names starting `__` without advancing this counter. A layout such as
`float DEBUG; float4 MaskTransform;` can put the vector in a later HLSL row while
the CPU writes at byte 4. This is a source-derived risk, not an executed failure.
Arrays/structs add further unsupported-layout questions; no need for them in this
MVP. Source: [ReflectVariable:112-127](../../Source/Engine/GraphicsEngine/RHI/RHIShaderReflectionInfo.cpp#L112)
and [top-level variables:191](../../Source/Engine/GraphicsEngine/RHI/RHIShaderReflectionInfo.cpp#L191).

**Conservative proposal:** four float4 rows (64 bytes) could carry signed
world-to-mask scale/bias, surface repeat/phase, noise controls and debug/fallback
controls for axis-aligned mapping. General rotated mask mapping takes two affine
rows instead of one, giving five float4 rows (80 bytes). Exposed Unreal vectors
already arrive as Vector4f. If retaining scalar MB_DEBUG, put packed vector rows
first and the scalar last, with no later reflected values, or explicitly pack
all subsequent scalars; validate actual reflection before relying on it. These
are layout options, not a final field contract or a workaround silently adopted
as a permanent rule. A targeted reflection fix is a separate implementation
option requiring normal verification.

## DDS data route and practical memory

The loader registers `.dds` only; it does not register PNG or automatically
convert/guess color space from the parameter name. GraphicsEngine reads bytes,
RHI calls `CreateDDSTextureFromMemory` with loader defaults, and DDS header
format controls the created resource. `DDS_LOADER_FORCE_SRGB` and IGNORE_SRGB
are conditional routes that this call does not request. Sources:
[AssetRegistry.cpp:25](../../Source/Engine/GameFramework/AssetHandling/AssetRegistry.cpp#L25),
[GraphicsEngine.cpp:371](../../Source/Engine/GraphicsEngine/GraphicsEngine.cpp#L371),
[RHI:670](../../Source/Engine/GraphicsEngine/RHI/RenderHardwareInterface.cpp#L670),
[loader defaults:1827](../../Source/Engine/GraphicsEngine/InterOp/DDSTextureLoader11.cpp#L1827),
[format conversion:1130](../../Source/Engine/GraphicsEngine/InterOp/DDSTextureLoader11.cpp#L1130).

Read-only header inspection of **all nine AGP runtime surface DDS files** found:

| Maps | Header facts | Bytes per file |
| --- | --- | --- |
| 3 BaseColor | 1024x1024, 11 mips, DX10 DXGI 99 (BC7_UNORM_SRGB) | 1,398,276 |
| 3 OcclusionRoughnessMetallic | 1024x1024, 11 mips, DX10 DXGI 98 (BC7_UNORM) | 1,398,276 |
| 3 Normal | 1024x1024, 11 mips, legacy FourCC BC5U (BC5_UNORM) | 1,398,256 |

Observed file total: **12,584,424 bytes**. Subtracting six 148-byte DX10 headers
and three 128-byte legacy headers gives **12,583,152 bytes of mip payload**, about
12.0004 MiB for the nine surfaces. Every BC5/BC7 mip uses 16-byte 4x4 blocks,
including minimum blocks for tiny mips. This is texture payload arithmetic, not
measured resident GPU allocation, driver overhead, streaming or bandwidth.
See [block accounting](../../Source/Engine/GraphicsEngine/InterOp/DDSTextureLoader11.cpp#L492)
and [BC5U decode selection](../../Source/Engine/GraphicsEngine/InterOp/DDSTextureLoader11.cpp#L870).

A prospective 512x512 RGBA8 linear mask costs 1 MiB for mip zero or 1,398,100
bytes for its full mip chain, excluding header/driver overhead. This format
would preserve independent 8-bit channels and avoid block compression artifacts
in the initial registration test. A separate noise image adds its own actual
payload. These are proposal calculations, not a selected mask/noise format.
Mips and filtering change overlapping alpha results at distance; they belong in
the visual comparison, not just a memory calculation.

The assets track reports `T_Noise01.dds` as DXGI 99 (sRGB). If used as blend
data through this loader, its samples receive the DDS sRGB behavior; simply
calling it noise in HLSL does not make it linear. A later contract must choose
the intended numerical noise values and prepare the matching format. Existing
presence of this file is not evidence that it is already a suitable mask/noise
resource. See [asset/geometry research](assets-geometry-research.md).

The content-owned ExportAssets.bat creates BC7 sRGB base colors, BC7 linear ORM
and BC5 unsigned normals with full mip chains. Its filename filters include
`_BaseColor`, `_C`, `_OcclusionRoughnessMetallic`, `_ORM`, `_Normal`; arbitrary
mask/noise names need an explicit preparation route. It deletes converted
runtime PNGs on success. Do not run it as a harmless inspection step.
Source: [ExportAssets.bat:43](C:/Users/tarik.bergstrom/Perforce/AGP-Content/ExportAssets.bat#L43),
[conversion:66](C:/Users/tarik.bergstrom/Perforce/AGP-Content/ExportAssets.bat#L66).

## Color, ORM and normal conventions

AGP `SurfaceValues.rgb` is AO/Roughness/Metallic. Roughness is clamped to
0.04..1 in lighting, so a lighting/debug comparison can hide differences below
the floor. Live function evidence now shows the same R/G/B wiring. Runtime ORM
DDS is linear, but the live Unreal textures have sRGB enabled and the function
uses a LinearColor sampler. The semantics track now establishes that the
connected TextureObject's Color sampler metadata overrides the function sample
in the checked compiler path. Combined with the source texture's sRGB resource
flag, this is consistent with sRGB-decoded ORM values; it does not imply a
compile mismatch or fallback. This is source-path inference, not a framebuffer
comparison. The owner accepted **linear ORM** as the AGP target, so an intentional
numerical difference from current Unreal settings is allowed. Do not claim
identical roughness/AO/metallic parity until comparing a corrected linear-data
reference. See [Unreal sampler analysis](unreal-semantics-research.md#linear-png-preservation-and-orm-exception).
Sources: [GBuffer output](../../Source/Engine/GraphicsEngine/Shaders/Material/GBuffer_PS.hlsl#L56),
[deferred lighting](../../Source/Engine/GraphicsEngine/Shaders/Material/Deferred_AmbientLight_PS.hlsl#L23),
[live function](audit-function.t3d#L68), [live settings](audit-texture-settings.json#L15).

Both existing AGP normal decode paths read RG, map to [-1,1], derive positive Z
with sqrt(saturate(1-dot(XY,XY))), and normalize. Neither flips green. This fits
the actual BC5U two-channel storage and the live normal textures'
`flip_green_channel=False`, but exact source/FBX tangent convention and rendered
orientation still need a directional test. Code/source settings agreement is
not visual proof. Sources: [GetWorldNormal](../../Source/Engine/GraphicsEngine/Shaders/Material/Lighting.hlsli#L196),
[Normal_DecodeRG](../../Source/Engine/GraphicsEngine/Shaders/Material/Normal.hlsli#L18),
[live normal settings](audit-texture-settings.json#L29).

## World coordinates and projected normals

Active actor and component parsing permutes transform axes so translation is
`(UE Y, UE Z, UE X)`; `UnrealUnitScale=1` retains numeric translation units.
Thus UE ground-plane XY corresponds to **AGP WorldPosition.zx**, in that order.
The vertex shader transforms imported positions by OB_World; the pixel hook
receives that world position and can independently replace final color, surface
values and world normal. Sources:
[component permutation](../../Source/Engine/GameFramework/UnrealSceneImporter/UnrealSceneImporter.cpp#L136),
[actor permutation](../../Source/Engine/GameFramework/UnrealSceneImporter/UnrealSceneImporter.cpp#L342),
[unit scale](../../Source/Engine/GameFramework/UnrealSceneImporter/UnrealSceneImporter.cpp#L560),
[vertex](../../Source/Engine/GraphicsEngine/Shaders/Material/Surface_VS.hlsl#L35),
[pixel seam](../../Source/Engine/GraphicsEngine/Shaders/Material/GBuffer_PS.hlsl#L30).

Expected requested 8-metre repeat is `WorldPosition.zx / 800` if combined FBX
and scene geometry uses centimetres. Existing connected Unreal function uses
**UV0/8**, which is a different rule; preserving that graph and adopting world
projection are separate acceptance targets. AGP copies the prebuilt FBX importer's
positions/UVs/directions; it does not prove its internal unit/axis conversion.
Sources: [mesh import](../../Source/Engine/GameFramework/AssetHandling/MeshAsset.cpp#L27),
[vertex copy](../../Source/Engine/GameFramework/AssetHandling/AssetLoadingHelper.cpp#L56),
[live function](audit-function.t3d).

The assets track sharpens the distinction: source FBX UV0 spans 0..56 over a
56-metre horizontal span, so UV0/8 can plausibly already provide seven 8-metre
repeats on this particular terrain. Raw FBX UV0 is `(local.x,-local.y)`, with
model translation `(-2800,+2800,0)` and scale 100. Do **not** equate those raw FBX
axes to Unreal world axes: Unreal import applies handedness/UV conversions, and
this asset uses Interchange import data. Likewise do not assume the prebuilt
AGP FBX importer preserves the raw V direction. See
[asset/geometry research](assets-geometry-research.md).

Root subsequently closed the Unreal side with read-only live sampling of all
4096 vertices. Actual Unreal local mesh UV0 is
`((2800-localY)/100, 1-(localX+2800)/100)`; UV1 is
`((2800-localY)/5700, 1-(localX+2800)/5700)`. UV0 spans U=0..56,V=-55..1,
with measured residual around 1e-14 for the fitted formula. With this actor's
identity XY transform, current **Unreal static-mesh** UV0/8 is consequently
`((2800-worldY)/800, 1/8-(worldX+2800)/800)`: U increases toward -Y, V toward
-X, seven repeats across 56m. This resolves the source mesh's metric repeat and
orientation; it is not the native Landscape's coordinate formula or an AGP
TGAFBX observation. A change to requested `WorldPosition.zx/800` projection would
change phase/orientation. Whether to preserve that instance's orientation or
choose explicitly aligned UE XY axes is an owner decision. Source:
[live mesh samples](audit-live-mesh-uv.json).

For a flat horizontal projected surface, UE +U/+V/+normal is AGP +Z/+X/+Y.
This orientation is internally right-handed: cross(+Z,+X)=+Y. A mask V flip
must be part of the world-to-mask transform; it does not automatically mean
flipping the surface normal's green channel. Surface texture projection and
mask orientation are separate transforms.

The mesh frame is currently built from imported tangent/normal (float3 without
a preserved tangent sign), transformed using inverse transpose, with bitangent
`cross(normal,tangent)`. Its orientation follows mesh UVs and transforms.
Simply using world-projected UVs while retaining that frame can rotate normal
details on a floater even when base color phase stays aligned. On nonuniform or
mirrored transforms there are further frame/sign concerns; do not claim support
without tests. Source: [Surface_VS.hlsl:54](../../Source/Engine/GraphicsEngine/Shaders/Material/Surface_VS.hlsl#L54),
[imported directions](../../Source/Engine/GameFramework/AssetHandling/AssetLoadingHelper.cpp#L109),
[Vertex layout](../../Source/Engine/GraphicsEngine/Objects/Vertex.h).

Candidate options, not selected:

- Preserve current mesh-UV/mesh-TBN behavior first for an exact graph baseline,
  then introduce requested projection with independent comparisons.
- For planar projection, construct a projection-consistent slope-preserving
  basis from geometric normal and known projected axes, or derive the appropriate
  basis from position/UV derivatives. Explicitly handle near-degenerate steep
  surfaces. A decoded flat normal must return the geometric slope, not global +Y.
  The semantics track now establishes the reference: ordinary arithmetic blend
  of decoded per-layer tangent normals, followed by final normalization and world
  conversion. Angle-corrected normal layering is a different operation and is
  not the existing Landscape alpha blend. Under projection, preserve that
  ordered blending in a shared projection-consistent frame and handle the zero
  vector according to the eventual fallback decision. Sources:
  [Unreal normal/blend analysis](unreal-semantics-research.md#exact-blend-order-fallback-and-normal-behavior).

No triplanar system or importer rewrite is justified merely by these observations.

## Ownership and smallest contract choices

Game-specific `MM_Landscape.hlsli` belongs in **AGP-Content/Content/Shaders/**;
general engine normal/binding helpers belong in AGP source. The build copies
AGP's runtime Content/Shaders first and engine source shaders second, so an
engine shader with the same relative name wins. `Bin/Debug/EngineShaders` is
generated. A persistent edit only in AGP's ignored `Content/` is not saved by
an AGP Git commit. No MM_Landscape.hlsli was found in the inspected AGP source,
runtime Content/Shaders or deployed Debug EngineShaders. Sources:
[ProjectVersionControl](../../Docs/ProjectVersionControl.md#L20),
[premake deployment](../../Source/Application/Game/premake5.lua#L79),
[actual project deployment](../../Source/Application/Game/Game.vcxproj#L126),
[missing shader selection](../../Source/Engine/GameFramework/Scenes/SceneWorldBuilder.cpp#L103).

| Proposal | Scope and tradeoff |
| --- | --- |
| Explicit exported parameter-name binding before legacy suffix fallback | Small general binder change, keeps existing asset names, makes layer role independent of filename. Shader declares stable per-layer names; original standard materials keep suffix behavior as fallback. Needs regression coverage for old standard maps, missing names, conflicts and actual three-layer assignment. Not implemented/authorized now. |
| Asset aliases with suffix-free DDS stems | Can use current custom-resource route without C++ changes; requires deliberately prepared duplicate/renamed runtime assets and matching resource names/metadata. Avoid `_BaseColor`/`_Normal`/long ORM suffixes for custom aliases. Adds asset maintenance and provenance burden; not selected. |
| Hardcoded material-specific bindings in engine/application | Could force references locally but would bypass reusable exporter contract and reintroduce scene-specific knowledge. No evidence it is needed; not recommended as the default. |

In either viable route, a minimum explicit contract must carry: three layer
identities and ordering; nine texture roles; raw/effective mask semantics;
mask channel order; linear format and mip policy; affine world-to-mask transform
including origin/scale/orientation/pixel registration; surface repeat/phase;
blend formula/fallback; noise source/mapping/strength; and projected normal rule.
Existing scalar/vector/texture records can carry much of this after resolving
binding and missing export values. That does not mean all fields already exist.

The exporter enumerates texture *parameters*, not arbitrary hardwired texture
sample/function-object references. It writes `AssetImportPath` by unconditionally
dereferencing `value->AssetImportData` for a non-null texture. A generated baked
texture therefore needs valid source/import metadata or an explicitly supported
export route; the current code is not proof that native/generated textures are
safe to enumerate. Null texture values write empty objects, while AGP expects
Name and AssetImportPath fields. These are source risks, not observed failures
or an explanation of an earlier editor exit. Source:
[TGALevelSerializer.cpp:304](C:/Users/tarik.bergstrom/Perforce/AGP-Content/Unreal/P5G4_KnightHeart/Plugins/TGAUnrealPlugin/Source/TGAUnrealPlugin/Private/TGALevelSerializer.cpp#L304),
[texture serialization:381](C:/Users/tarik.bergstrom/Perforce/AGP-Content/Unreal/P5G4_KnightHeart/Plugins/TGAUnrealPlugin/Source/TGAUnrealPlugin/Private/TGALevelSerializer.cpp#L381).

## Owner decisions and remaining evidence

Owner decisions were updated during synthesis:

1. **Resolved:** preserve Dunes -> Rough -> Cliff ordered alpha compositing.
   **Resolved:** linear ORM is the AGP data contract. These are accepted owner
   decisions, not implementation results.
2. Confirm adopting requested world-space 8m projection instead of the current
   UV0/8 graph, including how steep faces and rotated floaters should look.
3. Noise off must reproduce the chosen baseline. Should noise only reshape
   existing transitions, or may it introduce a layer where its painted weight
   is exactly zero? Recommendation: preserve absent layers unless specified.
4. Outside mask coverage, choose clamp-to-edge versus a specified fallback
   layer. A clamp sampler alone repeats edge values beyond terrain; it is not
   an explicit outside-coverage policy.
5. Choose which authoring contract to maintain after this debug instance:
   explicit material parameters/bake metadata versus a deliberately generated
   sidecar. Current debug colors and zero alpha are an intentional test setup.

Evidence still needed: precise bake linearity/orientation/texel centres, weight
order/fallback, current Unreal compile/ORM behavior, exact converted-mesh
registration/scale, source DDS/Unreal correspondence, noise selection and any
actual compiled/visual/performance result. Internal Landscape weightmap packing
is unnecessary for an explicit final-weight bake contract if that seam is
verified; do not make a fragile internal packing dependency without a need.

## Future meaningful visual verification matrix

These checks require a later authorized implementation/test phase. None ran here.

| Check | Observable acceptance / what it distinguishes |
| --- | --- |
| Binding identity | Force each of three layer weights to 1 with distinct color, roughness and directional normal markers. Each uses its own three intended textures; check logs for null/missing resource or fallback cube/material. Actual Dunes/Rough base colors are byte-identical, so require roughness and directional normal evidence for their actual assets; albedo alone cannot distinguish them. |
| Current paint baseline | Compare chosen Unreal surface result against AGP with noise=0 at pure, pairwise overlap, all-three overlap and zero-weight regions; include soft weights such as 0.25/0.5/0.75. Separate raw RGB-weight preview from composited debug color. |
| Numeric linear mask | Known 0, 0.25, 0.5, 0.75, 1 texels survive bake->file->DDS->sample within declared quantization tolerance. Inspect RGB numerically, independently of alpha/viewer opacity or tone mapping. |
| Registration | Asymmetric four-corner/centre markers agree between known Unreal positions, baked pixels and AGP terrain. Test half-texel placement, V flip, negative world coordinates, border and just-outside coverage. |
| Metric world phase | A measured 800-unit span has exactly one repeat on validated centimetre geometry; translating/rotating/scaling a mesh changes its sampling according to world position. Terrain and a differently UVed floater at the same world point share color/mask phase. |
| Normal projection | Flat map preserves geometric slope. A +U/+V directional normal lights in the expected world directions on terrain plus yaw-rotated floater; include steep surface and nonuniform scale, classify mirrored transforms explicitly. |
| ORM and color | Display raw linear AO/roughness/metallic channels and albedo with a controlled light/environment. Resolve Unreal sRGB/sampler discrepancy first; lighting similarity alone cannot establish channel equivalence. |
| Noise behavior | Off/on at the same camera and paint; strength zero equals baseline, transitions change visibly with strength, pure regions/fallback follow chosen policy, no swimming when camera moves, phase shared on floaters. |
| Distance/filtering | Near/far and oblique views check mip seams, mask border bleed, compression artifacts, aliasing and overlap behavior. Include paired alpha-mask/effective-contribution comparisons if precompositing is chosen. |
| Debug/render paths | Check final color, world-normal, roughness, metallic and AO views. Current local-normal debug can show the pre-hook standard normal sample, so it is not proof the custom projected normal is correct. Keep opaque material alpha separate from mask data alpha. |
| Cost | Record actual compiled reflected slots/constant offsets plus GPU timing for baseline and splat material under identical scene/camera/settings. Report hardware/resolution; source texture counts alone are not a performance result. |

For the debug limitation see [GBuffer_PS.hlsl:46](../../Source/Engine/GraphicsEngine/Shaders/Material/GBuffer_PS.hlsl#L46)
and [forward alpha discard](../../Source/Engine/GraphicsEngine/Shaders/Material/Lit_PS.hlsl#L53).
An opaque GBuffer shader and the forward/unlit paths do not treat alpha
identically; importing the debug's alpha=0 as a runtime surface output needs an
explicit decision. Layer weights themselves must not be derived from preview
opacity.
