# Unreal terrain import: exploratory plan

Status: historical exploration; superseded by the current museum planning record.

The current owner checklist and next-agent handoff is [Splat map MVP preparation](../../Docs/SplatmapMVPPlan.md). It supersedes this broader exploratory plan. Update 2026-10-22: AGP-Content and exporter source are now local; Unreal is downloading. The owner reports a teammate-authored museum splat terrain with three base-color/ORM/normal texture sets. Plan for all three layers and properties with noisy blend weights. The current museum JSON does not expose an identifiable three-layer splat material; verify the Unreal map and export path next. Keep the remaining text below as historical research, not current scope or environment status.

This is an exploration under agp-feature, not an approved implementation specification. See [research](research.md) for sourced terrain concepts, [pipeline investigation](pipeline-investigation.md) for engine/exporter evidence, [export inventory](export-inventory.md) for concrete assets, and [workflow](workflow.md) for continuation state.

## Agreed scope after clarification

- Required: splat-map material distribution and blending, with noise applied to blend weights, for surfaces such as snow, rock and grass.
- Optional bonus: texture-height-based material blending.
- Terrain mesh geometry supplies elevation when correctly exported; it is a separate prerequisite. Heightmap mesh generation, ground-height queries and collision are not requirements of the splat material feature.
- Current task: investigate pipeline support and needed data before writing an implementation specification.
- The newer Unreal/exporter pipeline is inaccessible on this device, as confirmed by the user. Do not ask for access again during this investigation.

### Revised order

1. Current checkout investigation: completed; see the linked evidence records. General FBX/material transport exists; native Landscape records do not supply geometry or maps in the checked exports.
2. On a device with the actual exporter, collect the small evidence package described in the pipeline investigation. Confirm a terrain mesh, mask mapping and named layer assets arrive intact. The newer exporter's support remains unknown until then.
3. Verify the mesh and display the mask as diagnostic color in AGP. This is the first later runtime checkpoint, not work performed in this planning session.
4. Plan and implement the required material blend plus noisy weights, using the existing material route where the verified inputs permit it. Define normalization, zero-weight fallback and whether noise may introduce absent layers. Select a stable noise mapping and controls.
5. Consider detail-height blending separately after the required visual result passes.

The options below preserve the earlier educational exploration. Option B (heightmap reconstruction) and C (ground queries) are alternatives for future needs, not part of the agreed minimum.

## The decisions we are separating

| Capability | Input | Observable result |
| --- | --- | --- |
| Terrain shape | Height samples plus placement/scale, or an already exported mesh | Hills and valleys appear in AGP |
| Painted surfaces | Layer weights/splat masks, layer textures, and mapping information | Grass, dirt, rock, etc. blend in the intended places |
| Ground-height queries | Accessible terrain geometry or height samples and a defined sampling rule | Gameplay can find the ground at a horizontal position |
| Material height blending | Per-surface detail heights and blend rules | Dirt can appear between stones without redefining overall terrain elevation |

These can be delivered separately. Surface weights do not uniquely determine elevation: identical grass/rock paint can cover either a flat field or a mountain.

## What AGP has today

Repository evidence checked on 2026-10-19:

- [SceneData](../../Source/Engine/GameFramework/Scenes/SceneData.h) describes static/skeletal meshes, lights, cameras, and other components, with no dedicated terrain/heightfield variant.
- [Importer types](../../Source/Engine/GameFramework/UnrealSceneImporter/UnrealSceneStructs.h) likewise contain no dedicated Landscape component type.
- The Landscape actor in [the sample Blockout export](../../Content/ExportedScenes/Lvl_Blockout_Level.json) contains a transform and a root scene component, with no height/weight file references. This establishes a gap in that sample, not a claim about every exporter.
- Landscape-named FBX assets exist under Content. Their names alone do not establish their topology, UV mapping, or suitability for material painting.
- [MaterialInterface](../../Source/Engine/GraphicsEngine/Materials/MaterialInterface.h) exposes named/numbered texture binding and a 16-slot material budget. [SceneWorldBuilder](../../Source/Engine/GameFramework/Scenes/SceneWorldBuilder.cpp) already applies imported texture parameters by name. Custom material vertex/pixel hooks exist. Investigate reuse before introducing a new material system.
- [AssetRegistry](../../Source/Engine/GameFramework/AssetHandling/AssetRegistry.cpp) registers DDS textures; the graphics RHI uses the DDS loader. Native height export formats will need an explicit CPU decode or offline conversion path; loading a GPU texture does not provide a ground-height query API.
- [Importer handoff](../../Docs/ImporterHandoff.md) states that the newer Perforce importer is absent. Do not infer its schema or coordinate conventions from these examples.
- The local exporter PDF has now been extracted and read using a temporary PDF library, which was removed afterward. It documents mesh references and material parameter types, but no dedicated Landscape geometry or painted weightmap payload. This does not establish what a newer exporter can do.

## Candidate first deliveries

### A. Paint an existing terrain mesh

Use a suitable exported terrain mesh and a verified weight map to blend two simple surfaces. This is a small route to learning splat mapping when the geometry already arrives correctly. It does not establish support for heightmap terrain reconstruction or CPU height queries.

### B. Reconstruct terrain from a heightmap

Import height samples and placement metadata, produce a bounded static terrain mesh, then add layer painting. This directly addresses terrain elevation if no suitable terrain mesh is exported. It requires more importer and geometry work than A.

### C. Make terrain usable by gameplay

Add a ground-height sampling contract after selecting A or B. Define supported transforms, edge/out-of-bounds behavior, holes, and interpolation relative to rendered triangles. This is needed for placement or grounding, not just to display terrain. It does not itself deliver a general character controller or collision system.

The clarified minimum follows A if the actual exporter supplies suitable terrain geometry. B and C require separate justification; do not add them automatically. Do not choose based only on asset names.

## Earlier exploratory milestone possibilities

1. **Verify one tiny Unreal export.** Capture the Unreal/exporter version, height or mesh data, two painted layers, transform, dimensions, axis/units, image orientation, and named layer mapping. Include an asymmetric hill or corner marker so mirroring and rotation are visible. Decide whether the first route is A or B from the actual files.
2. **Display the shape correctly in AGP.** Load that fixture through the existing scene/asset boundary; verify position, footprint, elevation landmarks, normals, and shadows. Diagnose unsupported inputs clearly. Preserve the existing candidate-world failure behavior on scene-load failure.
3. **Display two painted surfaces correctly.** Start with clearly distinguishable layer colors, then real surface textures. Verify pure layers, a mixed boundary, UV alignment, zero-weight fallback, and linear data sampling. Expand layer count only against a stated asset requirement and texture budget.
4. **Add gameplay sampling if selected.** Test known points, interpolation, transformed placement, and boundaries through the public behavior. Compare sampled height against rendered geometry.
5. **Broaden only against evidence.** Tiles, LOD, large worlds, holes, alpha/height blend fidelity, procedural Unreal materials, and streaming are separate scope decisions.

These earlier possibilities are superseded for the current minimum by the revised order above. After export evidence and remaining material behavior are settled, write the spec and slice the selected work into verifiable tickets with real blocking edges.

## Export contract to establish before coding

- Whether terrain arrives as an FBX mesh, exported height image, or custom packed data; which tool owns export/conversion.
- Height encoding/precision, dimensions, byte order where relevant, horizontal spacing, vertical scale/reference, and complete transform conversion ownership.
- Whether data is an exported combined heightfield or internal component textures with borders/packing; tile offsets and shared edges if applicable.
- Weight file format, linear color interpretation, resolution/orientation, layer names and channel allocation, blending semantics, and zero-weight behavior.
- Surface assets, tiling scales, and the required correspondence with Unreal. Exporting weights alone does not reproduce an arbitrary Unreal material graph.
- Whether holes, multiple Landscapes, World Partition, collision, and runtime edits are required initially.

## Verification plan after scope is agreed

Use existing importer/BuildWorld and material import tests where they exercise observable behavior; add tests for a public terrain import/query seam only when that behavior is selected. Fixtures should prove precision, orientation, transforms, weight interpretation, and useful failure diagnostics rather than duplicate internal helper formulas.

Build the solution and relevant test projects with v145 after code changes. Run a small fixture scene in `Bin/Debug/Game.exe`; compare the same marked landmarks and layer boundaries with Unreal, including lighting and shadows. Record numeric tolerances after the fixture establishes scale and precision. A successful build alone does not prove terrain correctness.

## Open decisions

The minimum is material blending with noisy weights, assuming geometry arrives as a mesh. The actual newer Unreal/exporter version, map export behavior, channel meanings, coordinate mapping and production layer count remain unknown. The user confirms that toolchain is inaccessible on this device. Use the evidence-package checklist when it becomes accessible. No gameplay use needs to be invented for the material feature.
