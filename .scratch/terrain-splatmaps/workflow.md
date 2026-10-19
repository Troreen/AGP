# Terrain splat maps: workflow continuation

## Request and constraints

Research what splat maps are and investigate the existing Unreal export/import path before implementation planning. Use agp-feature; this run is research and planning only. The user clarified the minimum: splat-map material blending with noise applied to weights. Texture-height blending is optional; geometry should arrive separately as a terrain mesh. Do not implement yet.

## Current stage

Preparation handoff complete. `Docs/SplatmapMVPPlan.md` is the owner's personal collection checklist and the next agent's context, not a graphics-team reading assignment. The owner will gather tools/assets/answers, then have another planning session with an agent before finalizing the implementation plan. All further work belongs on the device with the actual toolchain/assets. Prior two-layer/base-color-only recommendations are not settled delivery scope; prioritize a small implementation without unnecessary visual-quality loss. No implementation is authorized by this handoff.

## Repository state at start

- Original Git base: `02886bf09c023aade3085315e6fb13fde1104e19`.
- Starting branch: `shader-testing-scene`.
- Initial staged, unstaged, and untracked scope: empty (`git status --short`).
- Integration branch: `splatmap-blending`, created at the user's explicit request from `shader-testing-scene` at the original base. No commits or staging performed.
- Active worktrees: none created.
- Research owner: background agent `terrain_research`, scoped to `research.md` only.
- No staging or commits authorized.

## Records

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
- The newer Perforce importer is absent according to `Docs/ImporterHandoff.md`; its terrain contract is unknown.
- Required noisy blend weights and optional detail-height blending are separate material capabilities. Neither requires terrain displacement, heightmap generation or gameplay height queries.
- All 12 local scene JSONs were inventoried: 11 native Landscape records contain only root metadata; two landscape-named static meshes use the normal mesh/material route. No explicit splat-map contract was found.
- Bundled exporter manual was extracted and read. It documents mesh/material fields and parameter types but no dedicated Landscape map/geometry payload. Temporary PDF library downloaded for extraction was removed afterward.
- Existing importer can carry scalar/vector/texture inputs; named HLSL materials must implement the blending. Asset alias/name mismatches and fallbacks require explicit runtime checks.

## Verification

Read repository guides, scene records, importer/component types, material bindings, texture registration, FBX vertex conversion and DDS loading. Research cites primary Epic documentation. Extracted/read the bundled exporter manual and inventoried all 12 scene JSONs. Local Markdown links checked successfully across all five feature records; temporary dependency removal confirmed. `git diff --check` passed for tracked changes; new records were also read back. Planning changes require documentation/link checks, not game builds. No runtime terrain behavior has been implemented or verified. Final change scope is five new untracked planning/research records; no existing tracked files changed.

## Next action

The owner gathers the checklist inputs and records their paths/answers, then resumes a planning/overview session on the target device. The next agent inspects those inputs and finalizes the minimum useful implementation with the owner before coding. Two layers may prove the technique, but required layer count and normal/roughness blending remain open quality decisions. Do not ask graphics to read the plan or assume a new exporter must be written. Current documents are uncommitted; a branch by itself does not transfer these files to another device.
