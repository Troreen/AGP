# Independent runtime/assets evidence review

Reviewed2026-10-22 by the assets/geometry investigator, independently of the
runtime report's author. Scope: `runtime-contract-research.md`,
`material-graph-audit.md`, their raw snapshots, source binding/reflection/texture
load paths and the previously inspected file bytes. No editor queries, tests,
builds, conversions or code/asset changes were performed. This is an evidence
audit, not a declaration of implemented behavior.

## Findings requiring report correction or qualification

1. **Material-audit line references are inaccurate.** Its surface-coordinate
   section cites function lines139-153, beyond the current snapshot's end, and
   its ORM table cites81-87, which are function-output metadata rather than
   channel masks. Actual primary evidence is
   [audit-function.t3d:60](audit-function.t3d:60) for multiply,
   [68](audit-function.t3d:68) for BaseColor/metal/rough/normal/AO masks,
   [118](audit-function.t3d:118) for divide input, and
   [125](audit-function.t3d:125) for constant8. The conclusions themselves match
   these actual lines. Correct citations so a fresh reader can reproduce them.

2. **Runtime final gap list is partly stale.** Its owner-decisions section still
   lists precise bake orientation/texel centres and converted-mesh scale as
   wholly missing, although the seven pixel/paint queries and4096 mesh-vertex
   samples now provide bounded evidence. Replace this with the precise remaining
   gaps: untested transformed/edge coverage, DDS round-trip, AGP imported buffers,
   final compiled/visual comparison and owner behavior choices. Do not promote
   seven successful samples into universal bake proof, or Unreal vertex samples
   into AGP vertex verification.

3. **Live mesh coverage is first instance per vertex.**
   [audit-live-mesh-uv.json](audit-live-mesh-uv.json) explicitly records
   `vertex_instances_per_vertex_checked: first`. Current formulas/residuals
   establish all4096 sampled vertex positions/UVs and the quoted bounds. They do
   not constitute a census of every per-corner vertex instance, every LOD or the
   TGAFBX outputs. This is a qualification, not evidence that other instances
   differ. The assets note now records it explicitly.

No confirmed error was found in the material's layer identity/order, the DDS
format/memory arithmetic, suffix-binding obstruction or the candidate register
allocation. Remaining cautions below explain why those proposals need later
validation instead of being represented as ready behavior.

## Binding/register proposal: supported with explicit limits

Direct reads confirm
[SceneWorldBuilder.cpp:310](../../Source/Engine/GameFramework/Scenes/SceneWorldBuilder.cpp:310)
binds any `_BaseColor` stem to t0, `_Normal` to t1, long ORM suffix to t2 and
`_Emissive` to t3 before checking a custom name. The JSON parameter label is
unused in this branch. Therefore nine exposed texture parameters with current
names would collapse into three slots; adding metadata alone does not solve it.
The proposed binder change and suffix-free asset aliases are both real options,
neither implemented nor selected.

The all-custom t4..t12 surfaces,t13 mask,t14 noise allocation uses15 slots when
t0..t3 are retained, below the engine's16-slot material cap. Reserved lighting
resources in Lighting.hlsli are at t80,t98,t99,t100+, so they do not collide with
that allocation. Deferred GBuffer input t4 is used in different lighting passes,
not an additional occupied slot in this surface GBuffer pixel shader. Thus a
global search for t4 alone would be a false collision finding. This arithmetic
does not prove the chosen resources survive optimization, get correctly named
in reflection, or are bound by a future implementation.

[GraphicsEngine.cpp:1521](../../Source/Engine/GraphicsEngine/GraphicsEngine.cpp:1521)
binds the16-texture material vector starting at0 for VS/PS. Reflection merges
VS then PS and accepts the first same-name resource slot. Unique names and
consistent registers between both entry points are therefore part of the
future contract. Standard texture samples can remain in debug/emission paths;
the report correctly avoids claiming a compiled fetch count or performance.

The128-byte material constant limit and accumulated-offset reflection risk are
both present in source. Four/five packed float4 rows are plausible conservative
layouts, but should be checked against actual future compiled reflection.
No need for a larger resource cap, arrays or broad renderer refactor follows
from the current evidence.

## DDS/noise/conversion: supported, parity remains conditional

All nine observed DDS headers support the report: BaseColor DXGI99 sRGB,
ORM DXGI98 linear, normals legacyBC5U. File total12,584,424bytes and mip payload
12,583,152bytes are arithmetically consistent. No measured GPU residency or
timing is claimed. The512RGBA8 full mip calculation1,398,100bytes is also correct.

[RenderHardwareInterface.cpp:676](../../Source/Engine/GraphicsEngine/RHI/RenderHardwareInterface.cpp:676)
uses CreateDDSTextureFromMemory defaults; it does not suppress sRGB. Therefore
Noise01'sDXGI99 is an actual numeric-noise concern. Calling the same DDS a noise
resource would retain sRGB sampling. A later linear source/conversion decision
is necessary; it has not been executed.

The batch script's suffix filters and BC7/BC5 format flags match the report.
The helper's presence and read-only help output prove tool availability, not
successful conversion or a lossless RGBA-weight route. No file-name convention
currently gives arbitrary masks/noise dedicated numeric-data handling.

Important parity qualification: byte-identical content/AGP DDS pairs do not
prove identity to Unreal's cooked source pixels. Root's live texture settings
report all three Unreal ORM textures as sRGB=True, while runtime ORM DDS is
linear. The material function's LinearColor field alone cannot establish which
effective sampler metadata wins. Material audit appropriately defers to the
semantics source trace. Do not repair runtime ORM to sRGB without first deciding
whether to preserve the current Unreal result or intended linear ORM behavior.

Source Dunes/Rough BaseColor PNG/DDS bytes match each other and source RGB is
uniform(131,145,147); Rough ORM source is uniform(255,210,0). Any final visual
acceptance must include normal and material-channel evidence, since basecolor
cannot distinguish the two snow surfaces. This materially improves the existing
verification matrix without requiring another experiment now.

## Coordinate and material claims: supported with separate spaces

The live graph snapshots show Dunes textures3/4/5 through function0 and red,
Rough textures6/7/8 through function1 and green, Cliff textures0/1/2 through
function2 and blue. Layer blend order Dunes,Rough,Cliff is literal in
[audit-master.t3d:183](audit-master.t3d:183). Function ORM masks are R=AO,G=rough,
B=metal. No additional connected noise resource is present in these collections.

Actor/component scene translation permutation supports UE(X,Y,Z)->AGP(Y,Z,X),
so intended positive UEgroundXY projection corresponds to AGPWorldPosition.zx.
It does not prove that a proprietary FBX importer puts the mesh in the matching
coordinate frame. The source note and newer live Unreal sampling now distinguish
all three relevant spaces:

- Raw FBX: localUV0=(localX,-localY), model transform retained in the file.
- Actual Unreal static mesh: UV0=((2800-localY)/100,1-(localX+2800)/100).
- Native Landscape: gridUV0, and for this actor surface UV0/8=
  (worldXY+2800)/800.

Thus replacing either existing UV rule with absolute positive worldXY/800
changes source phase/orientation despite preserving an8m frequency. Current
paint-mask registration is an independent affine map and must not be inferred
from the second UV channel's57-sample denominator. The runtime report correctly
keeps projected normal basis separate from a mask V flip.

Finally, opaque Unreal debug Constant3Vector A=0 is stored editor color data,
but onlyRGB is connected to BaseColor. Treating that A as mesh opacity or a
baked alpha-mask channel would be unsupported. The graph audit distinguishes
this correctly. The approved weight bake independently leaves its unused A
channel0. No owner correction to the intended debug setup is required.
