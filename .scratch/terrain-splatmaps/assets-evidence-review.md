# Independent assets and material evidence review

Reviewed 2026-10-22. Scope: `assets-geometry-research.md` and
`material-graph-audit.md`, cross-checked against primary snapshots, current
import/binding/shader code and the separate Unreal semantics findings. This is
an independent review of those two reports, not an independent review of this
agent's runtime-contract report. No code, assets, settings, editor operations,
builds, rendering tests, conversions, Git or Perforce mutations occurred.
Read-only PNG pixel inspection was repeated successfully using System.Drawing.

## Concrete corrections for the report owners

1. **Assets report's UE vertex-sampling limitation is stale.** It says current
   loaded vertex samples were not measured and leaves the static-mesh import
   mapping pending. Root subsequently sampled all 4096 vertices (first vertex
   instance per vertex), with exact UV0 and UV1 formulas recorded in
   [audit-live-mesh-uv.json](audit-live-mesh-uv.json). Replace this limitation
   with the live result, keeping the proprietary **AGP** FBX conversion gap.
   Raw FBX UV `(x,-y)` is correctly qualified and must remain distinct from UE
   UV0 `((2800-y)/100,1-(x+2800)/100)`. Do not copy either formula onto native
   Landscape UVs. [Assets report](assets-geometry-research.md:112).

2. **Material report's function line references are incorrect.** The actual
   per-output pins are [audit-function.t3d:69](audit-function.t3d:69) through
   line73, not81-87. UV multiply is line60-62; divide/constant are line117-130.
   The snapshot ends at line135, so the claimed source lines139-153 do not exist.
   This affects traceability, not the reported UV0/8 or ORM R/G/B conclusions.
   [Material report](material-graph-audit.md:50).

3. **Two normal/ORM uncertainties have now been resolved by the source track.**
   Keep them distinguished from rendered verification. Unreal's current alpha
   layer node arithmetically blends decoded tangent normals, then the standard
   path normalizes and transforms the final normal; it is not angle-corrected
   detail-normal layering. Connected TextureObject metadata supplies the
   effective ORM sampler type, consistent with the source ORM assets' sRGB
   setting and no inferred compile fallback. These are source-path conclusions,
   not measured framebuffer results. [Semantics normal section](unreal-semantics-research.md#exact-blend-order-fallback-and-normal-behavior),
   [sampler section](unreal-semantics-research.md#linear-png-preservation-and-orm-exception).
   The owner has now accepted ordered Dunes -> Rough -> Cliff compositing and
   linear ORM for AGP; those are decisions, not evidence of runtime parity.

No new contradiction was found in the reports' actual connected material graph,
texture-role identities, independent painted masks, deliberate debug colors,
mask alpha explanation or runtime DDS header conclusions.

## Primary-evidence checks that held up

- Master function call inputs reference all three distinct snow texture sets;
  scalar DEBUG drives A=sampled attributes/B=RGB attributes in all three
  BlendMaterialAttributes nodes. The Landscape list is exactly Dunes, Rough,
  Cliff with LB_AlphaBlend. [Master inputs](audit-master.t3d:124),
  [layer order](audit-master.t3d:182), [debug blend](audit-master.t3d:212).
- BaseColor/Normal pins use RGB; ORM AO/Roughness/Metallic use R/G/B. All three
  function samples reference the shared UV multiply. [Function](audit-function.t3d:28),
  [attribute pins](audit-function.t3d:69).
- Debug Constant3Vector stores A=0, while the connected BaseColor input selects
  RGB. Opacity is not wired by those nodes. The instance snapshot selects
  DEBUG=1 and provides no texture/vector overrides. Thus PNG A=0 cannot be
  treated as sampled material opacity. [Debug RGB nodes](audit-master.t3d:188),
  [instance](audit-debug-instance.t3d:12).
- Re-read original diagnostic PNG at all seven documented positions. RGBA was
  `(255,0,0,0)`, `(255,0,9,0)`, `(255,253,255,0)`, `(0,0,255,0)`,
  `(0,0,24,0)`, `(7,255,14,0)`, `(0,0,255,0)` in the report's listed order.
  All RGB values match the material report. Comparing the recorded source
  weights times255 gives errors below one byte. Off-diagonal samples differ,
  so they challenge a simple transpose. This supports sampled image/world
  correspondence; it does not verify every pixel, boundary, DDS conversion or
  a rendered AGP mesh. [Source queries](audit-mask-point-queries.json).
- AGP actually copies the proprietary importer's returned UV0 and UV1, so the
  assets report is correct to leave the prebuilt importer's internal flips and
  node-transform handling unknown. [AssetLoadingHelper.cpp:98](../../Source/Engine/GameFramework/AssetHandling/AssetLoadingHelper.cpp:98).
- Current source reads normals from RG without a green flip and builds the mesh
  binormal from cross(normal,tangent). Therefore raw FBX normal/tangent vectors
  and texture flip_green=False alone cannot prove AGP projected normal parity.
  [Normal decode](../../Source/Engine/GraphicsEngine/Shaders/Material/Lighting.hlsli:196),
  [frame construction](../../Source/Engine/GraphicsEngine/Shaders/Material/Surface_VS.hlsl:54).

## Distinctive acceptance evidence still needed

The assets report correctly flags Dunes/Rough **identical uniform base colors**.
Neither normal/roughness binding nor blending can be proved by their ordinary
albedo screenshot. The runtime verification matrix now explicitly requires:

| Case | Required evidence |
| --- | --- |
| Dunes alone, Rough alone | Actual roughness views must differ according to their ORM textures; their actual directional normal detail must differ. Check the final world-normal output, not only the entry-point's original normal sample. |
| Dunes/Rough overlap | Compare known soft weights with ordered decoded-normal/roughness arithmetic. Use raw channel/world-normal diagnostics and controlled lighting; identical albedo is expected and is not a failed test. |
| All-three overlap | Full Cliff weight wins in the ordered rule; fractional weights retain the correct preceding contributions. Mask RGB preview may be white while composited debug color is blue. |
| Normal frame | Flat normal preserves geometric slope; a distinct directional +U/+V test on a rotated floater remains aligned with the chosen surface projection. Rotation/phase must use the eventually chosen native-Landscape/world anchor, not raw-FBX axes. |
| Zero coverage | Assert the eventual chosen fallback explicitly; no source report currently proves a useful normal for an all-zero blend. |

These checks are **future requirements**, not executed tests. See
[runtime verification matrix](runtime-contract-research.md#future-meaningful-visual-verification-matrix).

## Remaining gaps are bounded and explicit

The reports do not supply an AGP imported-vertex dump, compiled MM_Landscape
shader, end-to-end rendered alignment, linear DDS mask round-trip, performance
measurement or production material comparison. Mask outside coverage, noise
behavior, normal projection/phase and fallback were pending owner decisions at
review time. Internal component packing is correctly treated as heterogeneous;
the named-weight bake avoids relying on a single component's internal channels.
No finding here warrants implementation or an experiment without the owner's
existing required authorization.
