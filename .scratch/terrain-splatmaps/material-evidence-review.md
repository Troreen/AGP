# Independent material evidence audit

Reviewed 2026-10-22 against root's [material audit](material-graph-audit.md), raw
T3D/JSON evidence and installed UE5.7 source. No new editor queries, bakes,
rendering tests, asset changes or source changes were performed. Only this review
note was written. This pass reviews a separate author's synthesis; its reviewer
also authored the earlier Unreal semantics research, so the shader-semantics
analysis is a recheck rather than a second independent source investigation.

## Verdict

No material factual contradiction found in the connected graph, channel
assignments, LayerBlend order, separation of alpha meanings or sampled mask
registration. Two qualifications should remain explicit in the integrated
conclusions: blend results are source-derived material-property predictions,
and matching repeat size does not establish texture phase parity.

## Evidence checks and qualifications

**Graph and alpha are supported.** Raw master layers are Dunes/Rough/Cliff in
that order, each AlphaBlend. Constant3Vector values store A=0 but connect RGB
only to BaseColor. UE's Constant3Vector compiler emits RGB only, and default
per-layer Opacity is 1. The report correctly distinguishes those from both
painted interpolation alpha and the weight bake's unused fourth channel. It
must not imply the default per-layer Opacity=1 proves the blended Opacity
property always equals 1 under partial/zero coverage; the same arithmetic alpha
stack applies where that property is evaluated, while this opaque material's
visible coverage does not use translucent opacity. Sources:
[master:162](audit-master.t3d:162), [master:183](audit-master.t3d:183),
[master:192](audit-master.t3d:192),
[Constant3Vector compiler](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Engine/Private/Materials/MaterialExpressions.cpp:5126>),
[opacity default](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Engine/Private/Materials/MaterialAttributeDefinitionMap.cpp:340>),
[zero-filled packed alpha](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Landscape/Private/Landscape.cpp:3917>).

**“Resolves to blue” needs a precise interpretation.** The nested lerp formula
and effective coefficients are correct. At d=r=c=1, the debug **BaseColor
property** is blue by the checked source path. No framebuffer observation
establishes literal displayed RGB(0,0,1): lighting, exposure and tonemapping are
outside this derivation. The material audit calls the formula source compilation
evidence, so this is a presentation qualification rather than a wrong formula.
Source:
[LayerBlend.cpp:219](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Landscape/Private/Materials/MaterialExpressionLandscapeLayerBlend.cpp:219>),
[LayerBlend.cpp:254](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Landscape/Private/Materials/MaterialExpressionLandscapeLayerBlend.cpp:254>).

**World texture phase remains an explicit gap.** Native UV0 is Landscape-grid
XY, including section base, rather than absolute world XY. With the recorded
translation (-2800,-2800), scale100, identity rotation, UV0/8 corresponds to
`(worldXY + 2800)/800`. Absolute worldXY/800 would differ by 3.5 cycles on each
axis, i.e. half a tile after wrapping. Consequently “8 m repeat matches” cannot
be promoted into “source texture placement matches.” Root's note already
distinguishes world projection from the current graph; integrated planning
should retain a deliberate common phase/origin. Sources:
[native UV expression](<C:/Program Files/Epic Games/UE_5.7/Engine/Shaders/Private/LandscapeVertexFactory.ush:365>),
[section-base parameters](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Landscape/Private/LandscapeRender.cpp:2297>),
[live transform record](unreal-live-investigation.md),
[function:63](audit-function.t3d:63).

**Seven samples substantiate the limited stated claim.** The raw query JSON
contains seven pixel/world pairs and source weights. Multiplying those weights
by255 agrees with the source column in the report, including fractional values
such as Cliff≈9.38 and≈13.64, which help distinguish linear byte data from a
gamma-encoded result. The off-diagonal pair is asymmetric and distinguishes a
simple transpose. Root's comparison table records PNG bytes and says they were
read with System.Drawing; the JSON itself stores only source query values, not
PNG bytes/deltas. This is an artifact completeness limitation, not contrary
evidence. The report correctly limits the conclusion to checked points and does
not claim a DDS round trip or AGP mesh alignment. Sources:
[query values](audit-mask-point-queries.json),
[comparison table](material-graph-audit.md:122),
[pixel-center sampling](<C:/Program Files/Epic Games/UE_5.7/Engine/Shaders/Private/Landscape/LandscapeMergeTexturesPS.usf:75>).

**Bake format is settled for this diagnostic.** Root confirms the prior approved
MCP command explicitly created `RTF_RGBA8`, rather than `RTF_RGBA8_SRGB`.
Combined with source `IsSRGB=false` for that enum, byte-preserving PNG export
and seven numeric comparisons, this is sufficient evidence for the stated
diagnostic format/linear interpretation. Re-baking merely to rediscover the
released target's flags is unnecessary. This is source-plus-existing-sample
evidence, not a new exhaustive image test. Sources:
[RT format interpretation](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Engine/Private/TextureRenderTarget2D.cpp:71>),
[PNG export](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Engine/Private/ImageUtils.cpp:1266>),
[linear U8 unchanged](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/ImageWrapper/Private/ImageWrapperModule.cpp:494>).

**ORM warning is valid and should survive synthesis.** All three source ORM
assets are sRGB TC_DEFAULT in the recorded settings. A connected TextureObject
provides effective sampler metadata in the checked translator, overriding the
function sample's declared LinearColor. Do not claim compile failure solely
from those fields; equally, do not claim Unreal source sampling is linear data
merely from the function label. No framebuffer/parity validation was conducted.
Sources: [settings](audit-texture-settings.json),
[TextureObject metadata resolution](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Engine/Private/Materials/HLSLMaterialTranslator.cpp:4547>),
[effective sampler validation](<C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Engine/Private/Materials/MaterialExpressions.cpp:2885>).

**Minor source-pointer correction.** The material audit says function lines81–87
serialize all ORM masks; in the current raw snapshot, the relevant assignments
are lines69–73. Their contents establish exactly AO=R, roughness=G, metallic=B
as reported. This is a stale textual line range, not a channel-wiring error.
Source: [actual assignments](audit-function.t3d:69).

The review does not certify final shader output, performance, DDS conversion,
all-pixel mask fidelity, normal lighting parity or the external mesh conversion
history. Those have no new evidence in this review.
