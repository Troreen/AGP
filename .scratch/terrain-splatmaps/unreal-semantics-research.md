# Unreal Landscape semantics: source-backed findings

Investigated 2026-10-22. Research only; this agent made no code, asset, editor,
level, project-setting or version-control mutations. Engine source inspected at
`C:/Program Files/Epic Games/UE_5.7/Engine/`; live project log identifies version
5.7.4-51494982+++UE5+Release-5.7. Source references below are to that installed
version, rather than inferred from older Unreal tutorials.

## Findings that change the proposed contract

1. The three painted masks are independent inputs to ordered alpha composition,
   not three normalized barycentric weights. Normalizing their RGB sum changes
   this material's behavior.
2. The raw weight bake intentionally has unused alpha zero. That alpha does not
   come from the debug RGB material or its Constant3Vector alpha field.
3. Native Landscape UV0 is grid coordinates: with scale 100, UV0 / 8 means an
   800 cm repeat. The same formula on a converted mesh depends on its exported
   UV0 and is not automatically equivalent.
4. `render_weightmaps` currently supports 8-bit output formats only. Requesting a
   floating-point target is not an available precision upgrade in this source.
5. The ORM function sample's declared LinearColor is overridden by its connected
   texture object's sampler metadata in the checked compiler path. The currently
   sRGB-enabled ORM objects therefore need explicit consideration before using
   Unreal's shaded surface as a physically meaningful linear-data reference.

Each item is substantiated below; source inference is distinguished from live
observations and pending rendering checks.

## Material interpretation versus paint storage

Landscape material compilation obtains weights using layer-name-specific static
parameters. `StaticTerrainLayerWeight` samples material coordinate channel 3,
using a Masks sampler, then dots the sample with `LayerMask_<layer name>`.
Weightmap texture index and channel allocation are separate from the layer name.
There is no inherent rule that Dunes is R, Rough is G or Cliff is B internally.
Root's new component snapshots make this concrete: component 0 allocates only
Dunes to final texture Weightmap11/channel0; centre component 36 allocates
Cliff/Rough/Dunes to Weightmap69/channels0/1/2. Its edit-layer allocations are
also separate from final allocations. Sources:
[component 0 snapshot](audit-landscape-component.t3d),
[centre component snapshot](audit-landscape-centre-component.t3d).
Missing layer parameters return an invalid code outside the material preview;
the LayerBlend loop skips those entries. Preview weights only supply a default
when `Material->IsPreview()` is true. Sources:
[HLSLMaterialTranslator.cpp:9395](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Engine/Private/Materials/HLSLMaterialTranslator.cpp:9395>),
[LayerBlend.cpp:259](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Landscape/Private/Materials/MaterialExpressionLandscapeLayerBlend.cpp:259>).

Paint-time target-layer adjustment and material-time blending are independent
choices. UE5.7 LayerInfo's serialized `BlendMethod` is a private reflected
property with class default `None`. `None` is labelled No Weight Blending: its
weight is unaffected by other target layers. Other available values are
`FinalWeightBlending` and `PremultipliedAlphaBlending` (legacy and advanced weight
blending). Their enum tooltip contains an apparent `sum is 0` typo; do not use
that text as a numeric specification. Sources:
[LandscapeLayerInfoObject.h:261](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Landscape/Classes/LandscapeLayerInfoObject.h:261>),
[LandscapeEditTypes.h:32](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Landscape/Public/LandscapeEditTypes.h:32>).

New non-template LayerInfo objects obtain their initial BlendMethod from
LandscapeSettings; this project's `Config/DefaultGame.ini:590` explicitly sets
`TargetLayerDefaultBlendMethod=None`. Old assets migrate in `PostLoad` using
`FFortniteMainBranchObjectVersion::LandscapeAdvancedWeightBlending`: old
`bNoWeightBlend_DEPRECATED=true` becomes `None`; false becomes
`FinalWeightBlending`. Thus omission of a property from T3D alone is less useful
than direct property reflection. Root subsequently confirmed all three live
LayerInfo assets have `blend_method=NONE`. Sources:
[LayerInfo.cpp:16](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Landscape/Private/LandscapeLayerInfoObject.cpp:16>),
[LayerInfo.cpp:264](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Landscape/Private/LandscapeLayerInfoObject.cpp:264>),
[project configuration](<C:/Users/tarik.bergstrom/Perforce/AGP-Content/Unreal/P5G4_KnightHeart/Config/DefaultGame.ini:590>).

Epic's [Landscape material documentation](https://dev.epicgames.com/documentation/unreal-engine/landscape-materials-in-unreal-engine)
corroborates that layer names select target-layer weights, the graph determines
their interpretation, WeightBlend is order independent and AlphaBlend applies
overlays in list order. The installed version's source is used for the precise
formulas here.

## Exact blend order, fallback and normal behavior

The source first computes WeightBlend and HeightBlend contributions. For ordinary
arithmetic material attributes it initializes output to zero. Pure WeightBlend
is `sum(layer * weight)`; it does **not** divide by weight sum in this shader
node. HeightBlend modifies each height-layer weight as
`clamp(lerp(-1,1,weight)+height, 0.0001, 1)` and, when any height blend needs it,
renormalizes the non-alpha accumulation. Source:
[LayerBlend.cpp:154](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Landscape/Private/Materials/MaterialExpressionLandscapeLayerBlend.cpp:154>).

It then visits the layer list in order and applies every AlphaBlend layer as
`output = lerp(output, layer, weight)`. ShadingModel is a special discrete case;
the arithmetic formula applies to BaseColor, roughness, metallic, AO and Normal.
The current master T3D list is Dunes, Rough, Cliff, all `LB_AlphaBlend`.
Sources:
[LayerBlend.cpp:254](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Landscape/Private/Materials/MaterialExpressionLandscapeLayerBlend.cpp:254>),
[master snapshot](audit-master.t3d).

For current weights `d,r,c` and property values `D,R,C`, derived algebraically:

```text
result = lerp(lerp(lerp(0, D, d), R, r), C, c)
       = D*d*(1-r)*(1-c) + R*r*(1-c) + C*c
effective contributions = [d*(1-r)*(1-c), r*(1-c), c]
uncovered fraction      = (1-d)*(1-r)*(1-c)
```

These equations are an inference directly from the checked compiler loop, not
a measured framebuffer comparison. For diagnostic RGB layers, all weights 1
produce blue; `.5,.5,.5` produce `.125,.25,.5`, not gray. Raw mask RGB can be
white at an all-ones overlap because it stores independent weights rather than
the composited result. No automatic Dunes fallback is inserted. At all-zero
weights ordinary arithmetic attributes evaluate to zero; BaseColor is black and
Normal is a zero vector. A defined runtime fallback would be an intentional
behavior decision, not exact reproduction of this graph.

The blend node does ordinary arithmetic on the per-layer decoded normals; it
contains no reoriented-normal operation or per-layer normalization. The
standard material path subsequently normalizes tangent-space MaterialNormal and
transforms it into world space. Thus blending decoded tangent-space normals and
normalizing the combined result is the relevant baseline. A zero vector is
degenerate; no guaranteed useful lighting direction follows from it. Source:
[MaterialTemplate.ush:4276](<C:/Program Files/Epic Games/UE_5.7/Engine/Shaders/Private/MaterialTemplate.ush:4276>).

## Three unrelated meanings of alpha

The owner intentionally uses pure RGB debug colors with stored alpha zero.
The master snapshot confirms Constant3Vector values carrying A=0, but its
BaseColor pins select RGB only. Constant3Vector compilation emits exactly
`Constant3(Constant.R, Constant.G, Constant.B)`: its stored A does not enter the
shader. Each debug MakeMaterialAttributes node supplies only BaseColor;
unconnected per-layer opacity defaults to 1 and normal defaults to `(0,0,1)`.
Sources: [master snapshot](audit-master.t3d),
[MaterialExpressions.cpp:5126](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Engine/Private/Materials/MaterialExpressions.cpp:5126>),
[MaterialAttributeDefinitionMap.cpp:340](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Engine/Private/Materials/MaterialAttributeDefinitionMap.cpp:340>).

Separately, `LB_AlphaBlend` uses the painted layer mask as interpolation alpha.
Separately again, the three-layer weight export's fourth texture channel is
unused and explicitly filled from a dummy black texture. Its PNG transparency
therefore does not establish translucent surface rendering. This distinction
preserves the owner's debug intent while describing the actual pins and export
behavior. Source:
[Landscape.cpp:3917](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Landscape/Private/Landscape.cpp:3917>).

## Landscape UV0 units

Landscape's vertex factory sets UV0 to `LayerTexCoord`, calculated as local grid
XY plus component section-base XY and subsection offsets. C++ supplies
`SectionBase.X/Y` and subsection size in quads. Transform scale is not part of
this UV expression. Therefore one UV unit spans one Landscape quad; at the
observed XY scale 100 it spans 100 Unreal units. The connected UV0/8 surface
function repeats every eight quads, i.e. 800 cm with the current transform.
Sources:
[LandscapeVertexFactory.ush:365](<C:/Program Files/Epic Games/UE_5.7/Engine/Shaders/Private/LandscapeVertexFactory.ush:365>),
[LandscapeVertexFactory.ush:485](<C:/Program Files/Epic Games/UE_5.7/Engine/Shaders/Private/LandscapeVertexFactory.ush:485>),
[LandscapeRender.cpp:2297](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Landscape/Private/LandscapeRender.cpp:2297>),
[function snapshot](audit-function.t3d).

This resolves the native Landscape scale question. It does not prove converted
FBX UV preservation, mesh phase/orientation or world-projected normal basis.
Native UV phase is based on Landscape grid position, so changing to absolute
world XY/800 may introduce a constant phase offset from Landscape translation.

## Weight bake: data source, channel packing and limits

`ALandscape::RenderWeightmaps` delegates to `RenderMergedTextureInternal`, which
finds each requested target layer in each component's final
`GetWeightmapLayerAllocations()`, obtains the allocated texture and channel, and
merges/resamples those textures. It does not evaluate MM_Landscape, DEBUG,
BaseColor, opacity or lighting. The API itself handles internal allocation; a
bespoke extractor need not assume fixed internal RGBA channel assignments.
Sources:
[Landscape.cpp:3791](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Landscape/Private/Landscape.cpp:3791>),
[Landscape.cpp:3966](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Landscape/Private/Landscape.cpp:3966>).

Requested layers are packed in the supplied order. For the approved diagnostic
request `[Snow_Dunes,Snow_Rough,Snow_Cliff]`, output RGB has that mapping and A is
zero. The shader initializes output to zero and loads the per-channel source
textures; unavailable channels use black. Source:
[Landscape.h:362](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Landscape/Classes/Landscape.h:362>),
[LandscapeMergeTexturesPS.usf:109](<C:/Program Files/Epic Games/UE_5.7/Engine/Shaders/Private/Landscape/LandscapeMergeTexturesPS.usf:109>).

Accepted weight formats are `PF_G8`, `PF_A8`, `PF_R8G8`, `PF_A8R8G8B8`,
`PF_R8G8B8A8`, `PF_B8G8R8A8`. Float targets fail format validation. The
intermediate atlas and each resampled channel are `PF_G8`; output resolution
can increase sampling density but cannot recover finer underlying paint data.
The atlas removes duplicate subsection boundary rows and columns. It uses the
resident source mip, so source texture residency can affect capture fidelity.
Sources:
[Landscape.cpp:3634](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Landscape/Private/Landscape.cpp:3634>),
[Landscape.cpp:3514](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Landscape/Private/Landscape.cpp:3514>),
[Landscape.cpp:3821](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Landscape/Private/Landscape.cpp:3821>),
[Landscape.cpp:3892](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Landscape/Private/Landscape.cpp:3892>).

The method queues render commands; success is not proof that a file already
contains completed pixels. It can also return true when the requested world
area overlaps no component, without rendering. Keep an explicit completion and
nonempty-data check in any later exporter. Sources:
[Landscape.cpp:3741](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Landscape/Private/Landscape.cpp:3741>),
[Landscape.cpp:3863](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Landscape/Private/Landscape.cpp:3863>).

## Window registration and pixel centers

For valid extents, the supplied transform positions an XY rectangle local to
that transform. Invalid or zero-size extents switch to the loaded Landscape
bounds and Landscape transform. The code builds UV-to-world from transformed
`extents.min` and the transformed extent size. It offsets the internal atlas
origin by half a Landscape quad so source Landscape vertices lie at atlas texel
centers. Sources:
[Landscape.cpp:3727](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Landscape/Private/Landscape.cpp:3727>),
[Landscape.cpp:3839](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Landscape/Private/Landscape.cpp:3839>).

Output sampling uses pixel centers `SVPosition.xy / renderAreaSize`, transforms
these into atlas coordinates, checks `[0,1]`, and samples bilinearly with clamp.
There is no explicit Y inversion in this shader. Derivation for the approved
identity-transform window `[-2800,2800]^2` at 512 square:

```text
worldX(i) = -2800 + (i + 0.5) * 5600 / 512
worldY(j) = -2800 + (j + 0.5) * 5600 / 512
step = 10.9375 cm; first center = -2794.53125; last = 2794.53125
```

This is the source-backed registration formula. Root subsequently compared
seven pixel-center world queries to the existing PNG, including off-diagonal
locations: channel values agreed within one 8-bit step and did not require
transpose, Y flip or a gamma conversion at those samples. This verifies sampled
locations, not every pixel or the AGP mesh. See
[point query evidence](audit-mask-point-queries.json).
Shader samples outside the merged atlas return zero;
unallocated component atlas regions are cleared black. Sources:
[LandscapeMergeTexturesPS.usf:75](<C:/Program Files/Epic Games/UE_5.7/Engine/Shaders/Private/Landscape/LandscapeMergeTexturesPS.usf:75>),
[Landscape.cpp:3581](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Landscape/Private/Landscape.cpp:3581>),
[Landscape.cpp:3523](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Landscape/Private/Landscape.cpp:3523>).

## Linear PNG preservation and ORM exception

For render targets without an override pixel format, `IsSRGB()` is true only
for `RTF_RGBA8_SRGB`; ordinary `RTF_RGBA8` is linear. With override format, it
uses `!bForceLinearGamma`. The PNG export gets a render-target image; BGRA8
readback uses `RCM_MinMax`, labels the image using `IsSRGB`, copies read pixels
without a float-to-byte conversion, then compresses PNG. The ImageWrapper
explicitly leaves linear U8 bytes unchanged when its assumed gamma differs.
Therefore **if the diagnostic target was ordinary RTF_RGBA8**, the checked
source route preserves those mask bytes without a deliberate gamma transform.
That conclusion is conditional on the original target format and does not
prove subsequent DDS settings. Sources:
[TextureRenderTarget2D.cpp:71](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Engine/Private/TextureRenderTarget2D.cpp:71>),
[ImageUtils.cpp:499](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Engine/Private/ImageUtils.cpp:499>),
[ImageUtils.cpp:1266](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Engine/Private/ImageUtils.cpp:1266>),
[ImageWrapperModule.cpp:494](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/ImageWrapper/Private/ImageWrapperModule.cpp:494>).

The later seven-point comparison described above independently supports linear
numeric preservation for the existing PNG at the sampled locations. It is
stronger diagnostic evidence than judging the image visually.

Root's [live texture settings](audit-texture-settings.json) show all three ORM
assets `sRGB=true`, compression `TC_DEFAULT`. The master TextureObjects use
Color sampler metadata; the function's connected TextureSample declares
LinearColor. This does **not** establish a compile mismatch. With a connected
TextureObject, the checked compiler replaces sample-node sampler metadata with
that of the texture object before validation; translator comments explicitly
state sample-node texture/sampler fields are unused. TC_DEFAULT + sRGB selects
Color as the valid type. Sources:
[MaterialExpressions.cpp:2481](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Engine/Private/Materials/MaterialExpressions.cpp:2481>),
[MaterialExpressions.cpp:2885](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Engine/Private/Materials/MaterialExpressions.cpp:2885>),
[HLSLMaterialTranslator.cpp:4547](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Engine/Private/Materials/HLSLMaterialTranslator.cpp:4547>).

An actual mismatch at the effective sampler produces a compile error; it is not
silently accepted as another sampling mode. TextureObject's old compiler path
itself skips validation, while the consuming TextureSample validates the
effective type. Sources:
[MaterialExpressions.cpp:2713](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Engine/Private/Materials/MaterialExpressions.cpp:2713>),
[MaterialExpressions.cpp:4202](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Engine/Private/Materials/MaterialExpressions.cpp:4202>).

Texture resources use the asset's sRGB setting to choose sRGB creation flags;
D3D12 chooses shader-resource format from those flags. Thus the current ORM
path is source-consistent with sRGB-decoded samples, rather than assured raw
linear channel values. Existing AGP linear ORM DDSs can deliberately produce
different numeric roughness/AO/metallic from this Unreal setup. This is a
material-reference issue requiring an owner decision before claiming exact
shading parity; no settings were changed or shader compiled in this research.
Sources:
[Texture2D.cpp:868](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Engine/Private/Texture2D.cpp:868>),
[D3D12Texture.cpp:1121](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/D3D12RHI/Private/D3D12Texture.cpp:1121>).

## Remaining targeted checks

- Recovering the original diagnostic render-target enum would complete its
  instance metadata; its source behavior and seven numeric samples already
  support linear preservation. Any fresh bake beyond the approved temporary
  diagnostic scope requires permission.
- Compare asymmetric image/world locations to converted-mesh landmarks in AGP
  to complete end-to-end registration; image-to-Landscape sampling is already
  supported by the seven-point comparison.
- Decide ordered-alpha preservation versus deliberately normalized splatting,
  plus zero-coverage fallback and noise behavior. These are owner choices.
- Confirm whether Unreal's current sRGB ORM appearance is a target to preserve
  or an authoring setting to correct in later authorized work. A physically
  linear AGP contract should not be described as identical without checking.
- Confirm converted-mesh UVs/normal basis separately; native Landscape UV units
  do not establish FBX coordinate semantics. No visual runtime verification was
  performed by this agent.
