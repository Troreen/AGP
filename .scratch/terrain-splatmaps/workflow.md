# Terrain splat maps: workflow continuation

## School handoff ready, 2026-10-23

See [Docs/SplatmapSchoolHandoff.md](../../Docs/SplatmapSchoolHandoff.md) first.
Tickets 04–07 are resolved. Final run: 19 production checks + one isolated
experiment, zero test warnings/failures; Python guard tests pass. The explicit
UV-corrected reimport command now supports repeated source/sculpt updates.
The owner approved including the saved master/test-map edits. Full manual
from-zero/visual acceptance and runtime Front-X export remain owner checks.

Owner clarification during UV integration: normal scene-export mask coverage
validation is not wanted. The proposed check and test were removed; do not add
them back. Continue with strict UV preparation and same-mesh reimport.

## Latest resume: first-mask bootstrap, 2026-10-23

Ticket 06 resolved: new TGA creation/import action, derived bounds and effective
instance assignment; optional matching custom DDS names. All 15 integrated GPU
checks pass with zero test warnings/failures. Both reviews completed; naming
contract issue fixed. See [current continuation](continuation-2026-10-23.md).
Next is strict UV correction plus same-mesh reimport; full owner acceptance is
still pending. No shader feature or scratch cleanup work was added.

## Active continuation: material-instance export, 2026-10-23

Owner authorized continued implementation and requested a handoff plus commits
in both repos at 10% remaining. See [current continuation](continuation-2026-10-23.md)
and [ticket 04](issues/04-material-instance-export.md) before older handoffs.
Both Git trees started clean. Material-instance ticket 04 and paint-refresh
ticket 05 are resolved. Ten material-export checks, an authored-master copy
check and two fresh-Landscape GPU checks pass; independent reviews are complete.
The opt-in tool exposes the nine hardwired textures as connected parameters;
TGA can refresh an existing paint PNG without changing the mesh. First-mask
creation/import and integrated UV correction remain next. The real editor's
loaded plugin remains old; builds/tests use an isolated host to avoid changing
the open editor. The owner has not passed the fresh Landscape acceptance test.

## Latest handoff: 2026-10-23

User authorized committing/pushing both repositories to continue at school.
Use AGP `splatmap-blending` and AGP-Content `main`. Current continuation and
verification details are in `Docs/TerrainSplatmaps.md`; read its **Pick up here**
and **Required owner acceptance check** before the historical notes below.

Status: rendering demonstrated; authoring/export workflow incomplete. The owner
confirmed the prepared RGB baseline/noise comparison works, but explicitly does
not consider the feature working until they manually create a Landscape from
zero through sculpting, texturing, painting, Unreal export/reimport, normal TGA
scene export and use in AGP. The earlier mention of TM was corrected to AGP.
Follow `Docs/TerrainSplatmapFunctionalityTest.md` for the requested walkthrough
and record the first failing stage there.

Next: make that normal workflow usable, beginning with normal material-instance
parameter inheritance instead of duplicating artist settings in export metadata.
The exporter update currently serializes a complete opt-in export-data set;
those values override matching graph parameters, and child data replaces the
complete set. This limitation must be addressed in the next design iteration.
UV correction/mask refresh during the normal mesh conversion remains pending.
Do not delete investigation evidence before consolidating useful information.

The new TGA plugin built and passed four automation checks in Unreal 5.7.4,
including normal scene JSON export; its tested DLL was installed locally after
the owner closed Unreal. These ignored binaries must be rebuilt at school.
See AGP-Content `Docs/TerrainSceneExport.md`. RGB launch settings are regenerated
with `prepare-runtime-check.ps1` variants `Weights` and `NoiseWeights`.

## Request and constraints

Research what splat maps are and investigate the existing Unreal export/import path before implementation planning. Use agp-feature; this run is research and planning only. The user clarified the minimum: splat-map material blending with noise applied to weights. Texture-height blending is optional; geometry should arrive separately as a terrain mesh. Do not implement yet.

## Current stage

2026-10-22 implementation authorized: owner requested remaining questions and
implementation. Answers: splat mask covers the entire level (coverage validation
required); use a noise texture with strength and patch-size controls. Proceed
through spec/tickets/implementation/verification/review, without staging/commits.
Current implementation base is HEAD 517a6d7137f6fee2741d0a109b46126cf792062f;
original historical base remains recorded below. Branch splatmap-blending reused.
Initial AGP changes match the preceding investigation plus scratch correction
files; ApplicationSettings.json remains unrelated and must not be changed.
No staged files. Content main has many existing modifications/deletions and
untracked assets, including MM_Landscape and the test map: preserve all.
Perforce inspected with explicit clients: engine opened p4ignore.txt; content
opened T_Noise01.dds addition, p4ignore.txt, test map edit and museum JSON addition.
No Perforce reconcile/edit/submit performed. Content Git inspection required
escalated access and succeeded. Content writes require filesystem approval.
Spec: [spec.md](spec.md). Tickets: [01](issues/01-independent-layer-bindings.md),
[02](issues/02-three-layer-material.md), [03](issues/03-repeatable-assets-and-runtime.md).
Owners: engine_materials owns engine binder/reflection and its integration tests;
root owns shader/content preparation and shared records. No worktrees/commits.
Builds serialized through root. Prior research-only restrictions below describe
history and are superseded only within this newly authorized implementation.

2026-10-22 latest continuation: owner clarified that import yaw -90 is intentional
and must be preserved; the unwanted UV V flip must be removed. Owner explicitly
approved a scratch-FBX correction, separate unsaved import retaining -90 yaw, and
recording results. This approval does not cover production edits or the DDS test.
The experiment passed: all 4096 vertex positions and 6272 triangle connections
match the original; all 18816 vertex instances have corrected UV0/UV1. See
[result](audit-uv-corrected-yaw-minus90.json),
[semantic FBX comparison](audit-corrected-fbx-semantic.json), and
[procedure/result](asset-correction-procedure.md).
No source asset, level, material, settings or code was saved/changed by this run.
The editor level was already dirty before this experiment and remains dirty;
dirty content queries were empty before and after. The temporary mesh was not
placed. Final texture/normal appearance and AGP import remain unverified.
Next: discuss any further visual or production work before making those changes;
the independent DDS round-trip still awaits authorization. Older recommendations
below to remove yaw are superseded by this owner clarification.

Live investigation continuation: parallel tracks and independent cross-audits
are written and linked in [investigation-plan.md](investigation-plan.md).
Owner answered: ordered Dunes/Rough/Cliff, linear ORM, Dunes uncovered base,
transition-only noise preserving absent layers, original Landscape surface
mapping, and preference to correct export/import instead of shader compensation.
An approved unsaved temporary import with zero rotation confirmed the original
mesh's stored -90 yaw and separate V mismatch. Source assets remain unchanged.
Scratch corrected-FBX and DDS tests await their individual permissions.
Implementation/spec tickets remain deferred under the research-only constraint.

2026-10-22 continuation: owner explicitly requested a complete investigation plan
and parallel subagent research, using agp-feature with grill-with-docs. Current
scope remains research only; no code/source-asset edits. The debug instance's RGB
colors and zero alpha are intentional per owner. See
[investigation plan and assignments](investigation-plan.md) for active tracks,
initial Git scope, verification checklist and stop condition. Root owns live MCP;
unreal_semantics, runtime_contract and assets_geometry own disjoint research notes.
Current HEAD is 517a6d7137f6fee2741d0a109b46126cf792062f on splatmap-blending.
The earlier temporary mask bake was approved and succeeded; other experiments
require asking the owner first. Historical context below is retained as history.

Planning updated 2026-10-22. `Docs/SplatmapMVPPlan.md` is the current owner planning record and next-agent context. AGP-Content is now available beside AGP on this device, including the Unreal 5.7 project and TGA exporter source. Unreal is still downloading. The owner reports that their graphics teammate prepared realistic splat-map terrain in the museum scene with three complete base-color/ORM/normal texture sets. The target is three-layer blending of those properties with noise on weights; two-layer or base-color-only checks are intermediate diagnostics. The exact authored setup and its runtime export remain unverified. No implementation is authorized by this planning update.

## Current inputs and next verification

- Owner clarification: terrain geometry already follows Unreal authoring -> external software FBX conversion/preparation -> Unreal reimport -> placed mesh in scenes. Reuse ordinary mesh import; no native Landscape geometry exporter is required. Exact museum FBX/tool steps and how splat weights accompany the reimported mesh remain to be traced. Earlier findings about missing native Landscape handling describe a source limitation, not a geometry blocker for this established route.

- Additional owner requirement: surface textures use world-space ground-plane coordinates (Unreal XY), repeating once per 8 metres. Plan world-space mask sampling too, allowing terrain and ground floaters to share the shader and aligned distribution. Verify AGP axes/units, a shared mask origin/extent/orientation, edge behavior and normal-map basis. Keep mask coverage independent of surface tiling. Add measured-tiling and overlapping/rotated-floater visual checks; see the current plan's world-space mapping section.

- Engine checkout: `C:/Users/tarik/Documents/GitHub/AGP`, branch `splatmap-blending`; clean before this documentation update.
- Content checkout: sibling `../AGP-Content`; HEAD observed as `c9465d8`. Content working-tree status was not established because Git LFS attempted a temporary write outside the writable workspace; no content files were edited.
- Source map: `../AGP-Content/Unreal/P5G4_KnightHeart/Content/Levels/Test_Levels/SG/Lvl_Asset_Museum.umap`.
- Exporter source: `../AGP-Content/Unreal/P5G4_KnightHeart/Plugins/TGAUnrealPlugin/Source/`; plugin descriptor version 1.0, engine 5.7. Local plugin operation has not been tested.
- Runtime museum JSON: `../AGP-Content/Content/ExportedScenes/lvl_asset_museum/Lvl_Asset_Museum_Level.json`. Its master-material list is `MM_Snow`, `MM_Standard_WithSnow`, `MM_Standard`, `WorldGridMaterial`; no identifiable three-layer splat material is exposed. It may predate or omit the authored setup.
- Local landscape-named meshes and noise textures exist; museum texture candidates are traced below, while active assignments and mask mapping remain pending.
- Follow-up read-only investigation: museum package strings reference `/Game/Materials/MM_Landscape` and `Snow_Cliff`, `Snow_Dunes`, `Snow_Rough` LayerInfo assets. The master references nine `T_8M_Snow_*` textures; all nine runtime DDS files have valid headers and 1024 x 1024 dimensions. Active instance overrides and graph wiring remain unverified.
- The local serializer has no built-in Landscape handler or weightmap extraction. Its FBX routine is navmesh export. Geometry uses the owner's existing external FBX conversion/reimport workflow; ordinary material parameters can carry explicit mask references/mapping values. Verify an existing mask route or establish the smallest necessary mask bake/export step. Surface conversion scripts skip arbitrary mask/noise names, and local `texconv.exe` is absent. See the new investigation for details.
- While Unreal downloads: inspect source and existing exports for the museum transport path. Once ready: confirm the teammate's setup reached the local map, identify terrain actor/type and material, trace all nine textures and mask/channel mapping, produce a repeatable runtime export and capture a reference.
- Finalize the spec around the verified setup. Do not request a replacement teammate asset package or reopen the three-layer/map-property target without a concrete reason.

## Repository state at original research start

- Original Git base: `02886bf09c023aade3085315e6fb13fde1104e19`.
- Starting branch: `shader-testing-scene`.
- Initial staged, unstaged, and untracked scope: empty (`git status --short`).
- Integration branch: `splatmap-blending`, created at the user's explicit request from `shader-testing-scene` at the original base. No commits or staging performed.
- Active worktrees: none created.
- Research owner: background agent `terrain_research`, scoped to `research.md` only.
- No staging or commits authorized.

## Records

- [Current museum investigation](museum-investigation.md): package references, nine DDS candidates, exporter limitations, conversion gap, proposed material inputs and first-editor checklist. Supersedes older availability assumptions; no runtime/editor verification.
- [World-space mapping investigation](world-mapping-investigation.md): source-confirmed Unreal XY to AGP ZX conversion, expected 800-unit surface repeat pending geometry measurement, projected-normal implications and floater checks.

- [Owner checklist and next-agent handoff](../../Docs/SplatmapMVPPlan.md): collection to-dos, paths/answers record, minimum reusable test assets, quality guidance, verified context and continuation instructions. This supersedes earlier implementation suggestions in the exploratory records.
- [Research](research.md): official Unreal sources and beginner explanation.
- [Plan](plan.md): local evidence, proposed milestones, and unresolved decisions.
- [Pipeline investigation](pipeline-investigation.md): transport capabilities, limitations, required inputs and evidence package.
- [Export inventory](export-inventory.md): parsed scene census and concrete mesh/material/texture correlations.

## Decisions and evidence

- Separate terrain elevation, surface layer weights, material height blending, and gameplay ground-height queries.
- Do not treat a generic splat map as recoverable terrain elevation.
- Existing material hooks and named texture bindings may support a narrow splat material without a new renderer architecture.
- The current scene record variants have no terrain type. A checked sample Landscape actor supplies only transform/root data.
- Historical investigation: the newer Perforce importer was unavailable according to `Docs/ImporterHandoff.md`. Exporter source is now local; equivalence to other Perforce tooling and terrain transport remain unverified.
- Required noisy blend weights and optional detail-height blending are separate material capabilities. Neither requires terrain displacement, heightmap generation or gameplay height queries.
- The original 12-scene inventory found 11 root-only native Landscape records and two landscape-named static meshes using the normal route. This is historical checkout evidence, not a current AGP-Content census.
- Bundled exporter manual was extracted and read. It documents mesh/material fields and parameter types but no dedicated Landscape map/geometry payload. Temporary PDF library downloaded for extraction was removed afterward.
- Existing importer can carry scalar/vector/texture inputs; named HLSL materials must implement the blending. Asset alias/name mismatches and fallbacks require explicit runtime checks.

## Verification

Original research: read repository guides, scene records, importer/component types, material bindings, texture registration, FBX vertex conversion and DDS loading. Research cites primary Epic documentation. Extracted/read the bundled exporter manual and inventoried the original 12 scene JSONs; links and whitespace were checked then. Current investigation read exporter handlers and material serialization, parsed museum JSON, inspected selected Unreal package strings and nine DDS headers, and traced scene-coordinate conversion plus world-position/normal shader plumbing. Package strings establish references, not active material graph behavior. Planning changes require documentation/link checks, not game builds. No runtime terrain behavior has been implemented or verified. The current update changes planning documents only; no staging or commits authorized.

## Next action

Trace the existing museum setup rather than selecting or requesting a new terrain. File/source inspection can continue now; editor inspection, fresh exports and Unreal reference captures await Unreal installation. Establish exact layer identities, nine texture assignments, world-to-mask mapping, geometry route and export steps. Define noise behavior and weight rules, then finalize a short spec/task breakdown. Delivery target: three layers with base-color, ORM and normal blending plus noisy weights; world-space surface tiling once per 8 metres and intended shared world-space mask sampling for ground floaters. Do not assume a new exporter is required. Implementation starts only when requested.
