# Splat map MVP: preparation checklist and agent handoff

Status: gathering inputs. This is the owner's checklist and the next agent's continuation context. It is not a document for the graphics team to read or a finalized implementation plan.

## Goal and agreed requirements

Deliver a basic splat-map material implementation quickly, with reasonable visual quality. The required behavior is material distribution/blending controlled by a splat map, with noise applied to the blend weights for more natural transitions. Example surfaces are snow, rock and grass.

Terrain geometry is a separate input: the exported mesh should already contain hills and valleys. Texture-height-based material blending is an optional bonus, deferred from the MVP. Heightmap terrain generation and gameplay height queries are not requested.

All further pipeline investigation, implementation and testing should happen on the device containing the actual Unreal exporter/importer and assets. The immediate next stage is another planning session with the owner after the items below have been gathered. Do not start implementation automatically.

## Owner's to-dos before the next session

- [ ] **Make the actual tools available to the next agent.** Locate/sync the Unreal exporter/plugin, current engine importer and required project assets on the target device. Record local paths and the Unreal/exporter version or revision. Supply the current tools; building a new exporter is not a decided requirement.
- [ ] **Ask for one small representative terrain export.** Request the scene JSON, terrain FBX or equivalent mesh, its material assignment, and the source Unreal scene/setup. An existing terrain section is sufficient; no new production environment is needed.
- [ ] **Ask for the splat data and its meaning.** Obtain the mask image(s), which material each channel represents, and how the mask aligns with the mesh (UV channel or other mapping). Ask whether the exporter already produces these files or whether they must be baked/exported separately.
- [ ] **Gather the surface textures.** Obtain the base-color, normal and packed surface/roughness textures already used by the selected surfaces, plus a short description of their channels. Reuse existing assets. Missing optional maps should be recorded rather than delaying collection indefinitely.
- [ ] **Gather a noise source.** Obtain an existing suitable tileable grayscale noise texture, or record whether the intended material uses procedural noise. We will choose the simplest suitable approach after inspecting it.
- [ ] **Ask two scope questions.** How many materials must blend simultaneously in the first useful version? What would make the first result visually unacceptable: missing normal detail, wrong roughness, unnatural transitions, or something else? Record the answers; the team does not need to review this document.
- [ ] **Get a reusable export procedure.** Obtain the source setup and short steps/settings needed to change the mask and re-export independently. Confirm that you or the implementing agent can produce test variations without another graphics-team delivery.
- [ ] **Capture a reference.** Save one Unreal screenshot showing the chosen terrain and its material regions. Include a recognizable corner or landmark so mask alignment is easy to compare.
- [ ] **Bring the collected paths and answers to the next planning session.** Fill in the short record below. Unknown answers are acceptable; identifying what remains unknown is useful.

## Collection record

| Item | Path, answer or remaining unknown |
| --- | --- |
| Target project checkout and branch | |
| Unreal version / exporter location and revision | |
| Current importer location | |
| Source Unreal scene / export steps | |
| Exported scene and terrain mesh | |
| Splat mask(s), channel meanings and mapping | |
| Surface textures and packing conventions | |
| Noise source | |
| Minimum simultaneous layer count | |
| Visual quality requirements / reference screenshot | |

## Minimum reusable test setup

Use one small mesh with some visible slope, one material assignment and a mask with clear material regions and a gradual transition. Mark one corner asymmetrically to reveal flipped mapping. Keep the same mesh and surface assets while editing the mask.

Prepare or generate these variations on the target device; they do not require separate graphics deliveries:

1. Distinct solid material regions to check assignment and mapping.
2. A smooth transition to check basic weight blending.
3. The same transition with noise off/on to judge the change in blend weights.

A two-surface version is sufficient to prove the technique. The number of layers in the delivered MVP remains subject to the actual team requirement. Use existing texture resolutions initially. Runtime mask/noise textures must be treated as linear data; the checked-in engine texture route currently loads DDS.

## Quality and scope guidance for the next planning session

Keep the implementation small, but do not assume base-color-only blending is the finished result. Collect available normal and roughness maps now. Decide whether to blend those surface properties after inspecting the assets and visual target; reusing those maps may provide substantial quality without a large new system.

Prefer one material using existing mesh, texture and parameter plumbing. Preserve the painted distribution and give noise a visible, adjustable effect on transitions. Noise strength zero should allow a direct comparison with the original mask. Keep terrain-wide mask mapping separate from repeating surface texture tiling.

No advanced terrain system, arbitrary layer framework, editing UI or hot reload is needed. Height blending remains deferred. Exact channel layout, layer count, noise function, controls and shader formulas are not finalized in this document.

## What we already know from this device

- The existing FBX route imports positions, UV0/UV1, normals, tangents and material slots. Geometry and mapping fidelity for the actual terrain have not been tested.
- The current importer carries scalar, vector and named texture parameters. Material hooks expose UVs and world position, so explicit splat inputs may fit the existing route.
- AGP uses its own HLSL material selected by master-material name; it does not translate Unreal's material graph.
- The material interface has a 16-texture budget and a 128-byte parameter block. Allocate within the existing system before proposing architectural changes.
- All 12 checked-in scene exports were inventoried. Their 11 native Landscape records have only root metadata, with no geometry/maps. Two landscape-named static meshes use the ordinary mesh path. Asset names alone do not prove a usable terrain export.
- No explicit splat contract or noisy splat shader was found. Existing snow/noise parameter names do not establish implementation.
- Some exported texture names do not match local assets, and missing meshes/materials can use fallbacks. Successful scene loading alone does not prove the intended data was used.
- The bundled exporter manual was read, but the newer exporter/importer is inaccessible on this device. Its support is unknown, not assumed absent.

Evidence: [pipeline investigation](../.scratch/terrain-splatmaps/pipeline-investigation.md), [export inventory](../.scratch/terrain-splatmaps/export-inventory.md), and [terrain concepts research](../.scratch/terrain-splatmaps/research.md).

## Next agent: resume here

1. Read this checklist and [workflow state](../.scratch/terrain-splatmaps/workflow.md). Read the repo's AGENTS.md and use the repo-local agp-feature workflow.
2. Inspect Git status, current branch, supplied paths, actual importer/exporter and representative assets. Preserve existing work. The planning branch was created as `splatmap-blending` from `shader-testing-scene` at `02886bf09c023aade3085315e6fb13fde1104e19`; do not assume another device has the same checkout state.
3. Review the owner's collected answers. Verify facts from the supplied files before asking questions. Do not assume the older checkout's schema matches the newer toolchain.
4. Hold the requested planning/overview session. Settle the minimum layer count, surface properties needed for acceptable quality, actual asset mapping and smallest necessary importer changes. Recommend concrete defaults based on the package rather than a broad terrain roadmap.
5. Finalize a short implementation spec and task breakdown from those decisions. Proceed to implementation when the owner requests it; this handoff is not implementation authorization.
6. Later verification should show the correct mesh/mask alignment, expected layer regions, a visible noise-off/on difference and acceptable lighting. Use a small scene and screenshots, build the relevant projects, and reuse existing test seams where useful. Do not invent a large testing framework.

No engine code has been changed or runtime splat behavior verified. This branch and its documents are currently uncommitted; transfer the files explicitly or commit them only if the owner requests it.
