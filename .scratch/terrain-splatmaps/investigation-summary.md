# Splatmap investigation synthesis

Status: research in progress; the scratch UV correction test passed with required
yaw -90 preserved. DDS round-trip still awaits approval. No implementation is authorized. Updated
2026-10-22. This is the current synthesis; earlier notes retain historical facts
and are superseded where the live/source evidence below resolves their gaps.

## What the current setup actually contains

The active test scene is `/Game/Levels/Test_Levels/SP/Lvl_Splatmap_Testing`.
Native Landscape and converted mesh share the debug instance, whose parent is
`/Game/Materials/MM_Landscape`. Complete live snapshots expose 23 master nodes
and the 12-node MF_LandscapeLayer. There are three hardwired complete snow
surface sets, no connected noise, and UV0/8 surface sampling. ORM wiring is
R=occlusion, G=roughness, B=metallic. DEBUG=1 substitutes RGB BaseColor attributes.
Source: [complete graph audit](material-graph-audit.md) and linked raw snapshots.

The stored debug colors have A=0, intentionally. The shader connects only RGB
from those Constant3Vector nodes; alpha blending uses painted layer masks. The
weight-export PNG has a separate unused alpha channel of zero. These three
concepts are distinct; preview transparency does not mean absent mask data.
Source: [Unreal semantics](unreal-semantics-research.md).

Native painting supplies independent Dunes/Rough/Cliff masks. LayerInfo uses
BlendMethod=None and the master applies ordered alpha blends Dunes -> Rough ->
Cliff. Internal texture/channel allocations differ across components and edit
versus final data. The named-layer render API repacks them into an explicit
output order; direct use of one internal RGB texture would be unreliable.
Sources: [semantics](unreal-semantics-research.md), component snapshots in graph audit.

## Facts verified beyond filenames

| Question | Result and practical limit |
| --- | --- |
| Can paint be extracted? | Yes. Approved 512x512 RTF_RGBA8 render_weightmaps test returned true and produced nonempty RGB masks. Dunes/Rough/Cliff are requested RGB, not assumed internal channels. |
| Are mask values linear and oriented correctly? | UE source establishes linear RGBA8/PNG path and pixel-centre transform. Seven checked pixels, including asymmetric positions, match Landscape queries within one byte. DDS and final AGP rendering remain unverified. |
| Why did the PNG look empty? | Unused A=0 caused transparency. An opaque preview preserves every RGB byte; all pixels were compared. |
| Are source maps complete? | All nine source PNGs and runtime DDSs exist. DDS copies match between content and AGP. Base color is BC7 sRGB, ORM BC7 linear, normals BC5U. |
| Is basecolor-only validation enough? | No. Dunes and Rough base colors are byte-identical uniform images; roughness and final normal checks are necessary. |
| Does native UV0/8 mean 8m? | Yes for current Landscape scale100: one UV unit per 1m quad, eight quads per repeat. World phase is anchored at (-2800,-2800), not absolute origin0. |
| Is the converted mesh equivalent? | Not currently in orientation. Live data exposes rotated/flipped UV-versus-position mapping, not a changed repeat size. |
| What caused that difference? | Saved import Yaw=-90 rotates geometry. Separately, Landscape FBX export omits the V compensation used by StaticMesh export, while mesh import performs 1-V. |
| Did zero rotation alone fix it? | Approved unsaved temporary import restored geometry axes. All 4096 vertices and 18816 corners confirm remaining UV0=(nativeU,1-nativeV). Original source mesh was untouched. |
| Is terrain provenance known? | Current runtime source FBX MD5 matches the Unreal asset's recorded source MD5. It records Unreal export; historical external-DCC steps are not established by this file. |
| Does current JSON deliver splatting? | No. It carries mesh/material identity and DEBUG only. Native Landscape carries root metadata only. Hardwired graph textures and painted masks are absent. |
| Is the current terrain deployed into AGP? | Exact test FBX is absent there. The older similarly named _01 file is a different 126m terrain; current terrain is 56m. |
| Can nine maps fit the renderer? | Yes arithmetically: all-custom nine maps + mask + noise + four default slots =15/16. Existing suffix binding would nevertheless overwrite the layers into t0/t1/t2. |
| Can numeric settings fit? |128-byte block is sufficient for a small packed contract, but current reflection ignores HLSL StartOffset/padding. Actual layout must be validated in implementation. |
| Is noise already implemented? | No connected noise. Available Noise01 DDS is sRGB, unsuitable for assumed raw linear numeric sampling without explicit handling. |

Evidence: [graph](material-graph-audit.md), [semantics](unreal-semantics-research.md),
[assets](assets-geometry-research.md), [runtime](runtime-contract-research.md),
[UV cause](uv-orientation-cause.md), [temporary import](audit-zero-rotation-import.json).

## Settled owner decisions

1. Preserve the existing Dunes -> Rough -> Cliff ordering.
2. Use linear ORM data in AGP. Current Unreal sRGB ORM settings are an authoring
   discrepancy, not the shading-parity target.
3. Fill uncovered contribution with Dunes instead of reproducing black gaps.
4. Restrict noise to transitions. Preserve weights exactly at0 and1; never
   introduce a previously absent layer. Strength0 reproduces the chosen baseline.
5. Match native Landscape surface placement: common world XY projection,
   8m repeats, native Landscape phase shared with terrain/floaters.
6. Correct export/import asset preparation before considering an asset-specific
   shader compensation. This does not cancel the desired shared world projection.
7. No source code changes. Source asset changes remain outside this research
   scope; approvals for temporary tests do not authorize production reimports.
8. Latest clarification: import yaw -90 is intentional and must be preserved.
   Remove only the unwanted UV V flip. Earlier proposals to remove yaw are
   superseded; rotation changes orientation without corrupting UVs.

For original d,r,c masks the reference arithmetic is:

```text
reference = D*d*(1-r)*(1-c) + R*r*(1-c) + C*c
uncovered = (1-d)*(1-r)*(1-c)
```

The approved Dunes-fill policy adds `D*uncovered`. Consequently Dunes becomes
the continuous base: `D*(1-r)*(1-c) + R*r*(1-c) + C*c`. The original Dunes mask
remains useful evidence/debug data, but no longer changes final coverage under
that policy. Keep this intentional consequence visible in a later spec.
Noise formulas and controls may be chosen/tuned later to obey the settled
endpoint/absence rules; no noise implementation is selected by this research.

Outside the world mask, propose zero painted overlay weights and the same
Dunes base. This avoids extending arbitrary edge paint via clamp alone.
Surface textures wrap; mask coverage remains a separate world transform.

## Export/import correction path

The inspected FBX has no model rotation/rotated parent. A temporary import with
zero yaw verified the recorded -90 import offset as the rotation cause, while
leaving a separate V inversion. Removing only yaw is not a complete fix.

The owner-approved asset-preparation test made a scratch FBX with UV V
precompensated using the ordinary StaticMesh FBX convention, then imported with
the required yaw -90. All 4096 vertex positions and 6272 triangle connections
match the original exactly. All 18816 corners have corrected UV0/UV1; UV0 U is
unchanged and corrected V equals 1 minus original imported V exactly. UV1 has
only float-rounding differences (maximum 5.97e-8). The original source checksum
and original asset's yaw were unchanged. See
[semantic FBX check](audit-corrected-fbx-semantic.json),
[import measurements](audit-uv-corrected-yaw-minus90.json), and
[procedure](asset-correction-procedure.md).
Normals and Mikk tangents are recomputed, but final normal-map appearance,
metric repeat/phase in the renderer and AGP importer behavior remain unverified.
Do not blindly flip texture green.

The test must not overwrite the current FBX or mesh. Any final corrected export
belongs in AGP-Content, with source provenance and repeatable settings, followed
by explicitly scoped deployment. The exact AGP TGAFBX importer's internal
conversion is closed-source here and remains an end-to-end validation boundary.

## Narrow future implementation work, not authorization

1. Prepare the correct terrain asset and linear named mask with explicit channel,
   world coverage, pixel-centre and edge metadata; preserve source provenance.
2. Expose/serialize the nine texture roles and mask/control values, or use a
   deliberately documented equivalent metadata route. Current material graph
   translation is not required and does not already exist.
3. Resolve parameter-name versus legacy suffix texture binding. Prefer a small
   explicit-role route with legacy fallback; suffix-free asset aliases are an
   alternative with maintenance cost. No arbitrary-layer framework is needed.
4. Implement one content-owned MM_Landscape material: three surface properties,
   ordered layers/Dunes fallback, world mapping and transition noise. Validate
   packed parameter offsets and reflected texture slots within existing budgets.
5. Define a slope-preserving projected normal basis for the same world axes;
   check terrain and rotated floaters. Mesh UV tangent frames cannot be assumed
   correct after adopting world projection.
6. Run the [runtime verification matrix](runtime-contract-research.md), including
   final normal/roughness views, mask registration, pure/overlap/empty regions,
   noisy transitions, near/far filtering and measured performance.

Keep game-specific shader code in AGP-Content, engine binding changes in AGP,
and production source/runtime assets in AGP-Content. No shader files, renderer
code, project settings, source assets or runtime deployment were changed here.

## Review and limits

Independent reviews: [material](material-evidence-review.md),
[assets](assets-evidence-review.md), [runtime](runtime-evidence-review.md).
They confirmed central findings and identified stale uncertainty/citation
corrections, which are being reconciled in the linked reports.

Remaining relevant evidence: PNG/DDS numeric round-trip, final corrected-asset
appearance, actual AGP TGAFBX buffer/geometry registration, production reference
renders, projected-normal visual correctness, shader reflection/build and GPU
cost. The latter checks depend on later implementation/test authorization;
source inspection cannot substitute for them. Unrelated assets, history or
arbitrary graph permutations are outside this investigation's stop condition.
