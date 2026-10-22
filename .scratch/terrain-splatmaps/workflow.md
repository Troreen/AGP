# Terrain splat maps: workflow continuation

## Request and constraints

Research what splat maps are and investigate the existing Unreal export/import path before implementation planning. Use agp-feature; this run is research and planning only. The user clarified the minimum: splat-map material blending with noise applied to weights. Texture-height blending is optional; geometry should arrive separately as a terrain mesh. Do not implement yet.

## Current stage

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
