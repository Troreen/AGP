# Live Unreal splatmap investigation

Follow-up: [complete graph audit](material-graph-audit.md) resolves the initial
ORM mask and internal weight-allocation gaps below and distinguishes stored
debug constant alpha from shader opacity and unused baked PNG alpha. The owner
confirmed the debug RGB/zero-alpha values are intentional. See the newer audit
for full snapshots and pixel-centre checks; retain this note as session history.

Observed 2026-10-22 through Unreal MCP and local files. Research only, followed by
an owner-approved temporary diagnostic bake: no source material, asset, code or
setting changes. Unreal was reopened by the owner after
an inspection query passed an empty Base Color node to an API and crashed the
editor; subsequent graph traversal guarded empty inputs. No save was requested.

## Current scene and material

The live project is `C:/Users/tarik.bergstrom/Perforce/AGP-Content/Unreal/P5G4_KnightHeart/`.
The current map is `/Game/Levels/Test_Levels/SP/Lvl_Splatmap_Testing`, superseding
the museum as this pass's concrete test case. Both `Landscape_0` and static mesh
actor `SM_Landscape_Test` use
`/Game/LevelPrototyping/Materials/MI_Landscape_SplatMapTesting`, whose parent is
`/Game/Materials/MM_Landscape`. Sources: live actor/component reflection and
material instance `parent` property through MCP.

The instance has `DEBUG=1`, no vector overrides and no texture overrides. The
master's only named scalar parameter is DEBUG, default 0. The instance also
contains RefractionDepthBias=0, which was not listed as a master parameter.
Sources: instance parameter arrays and MaterialEditingLibrary parameter queries.

The connected Material Attributes output is `MaterialExpressionLandscapeLayerBlend_0`.
Its inputs are ordered Snow_Dunes, Snow_Rough, Snow_Cliff; all three are
`LB_ALPHA_BLEND`, with preview weights zero. This is current editor evidence,
superseding older package-string evidence suggesting weight blending.
Sources: `get_material_property_input_node`, guarded recursive
`get_inputs_for_material_expression`, blend node `layers` properties.

| Landscape layer | Surface texture prefix under /Game/Textures/ | DEBUG base color |
| --- | --- | --- |
| Snow_Dunes | T_8M_Snow_Dunes | Red |
| Snow_Rough | T_8M_Snow_Rough | Green |
| Snow_Cliff | T_8M_Snow_Cliff | Blue |

Each prefix supplies `_BaseColor`, `_OcclusionRoughnessMetallic` and `_Normal`.
Each layer calls `/Game/Materials/MF_LandscapeLayer`, then a BlendMaterialAttributes
node chooses between those surface attributes and debug attributes using DEBUG.
The debug branch supplies constant base color and default remaining attributes.
These colors identify debug layers; they do **not** establish internal weightmap
channel assignments. Source: live connected master graph traversal.

## Surface coordinates and properties

MF_LandscapeLayer samples its three texture inputs with the same coordinates:
`TextureCoordinate(UV0, tiling 1,1) * (1 / 8)`; the divisor's connected constant
is 8. This is UV-based, not world-space ground projection, and does not by itself
prove an 8-metre repeat on the converted mesh. No noise or world-position node
was found in the connected master/function path. Source: live traversal from
`MaterialExpressionFunctionOutput_0` through the function's connected nodes.

Base color and normal samples feed their corresponding material attributes.
The ORM sample feeds AO, roughness and metallic. Sample types are Color,
LinearColor and Normal respectively; the master enables tangent-space normals.
Exact ORM output channel masks and normal green-channel convention remain
unverified: the input-list API identified sample nodes but not each output mask.
Do not silently assume channel wiring from the texture filename.

## Painted weights and geometry

Landscape painting stores per-layer data on the Landscape; the layer blend reads
that data without an explicit user-assigned RGB mask texture. Live target layer
names are Snow_Cliff, Snow_Rough, Snow_Dunes. Map-specific matching LayerInfo
assets exist under `/Game/Levels/Test_Levels/SP/Lvl_Splatmap_Testing_sharedassets/`.
Sources: `Landscape.get_target_layer_names`, asset listing and LayerInfo layer names.

Read-only `editor_get_paint_layer_weight_by_name_at_location` queries across the
64 components found these values (maximum returned across components; sparse
diagnostics, not an image reconstruction):

| Unreal XY | Dunes | Rough | Cliff |
| --- | --- | --- | --- |
| -2400,-2400 | 0.35294 | 0 | 0 |
| -1400,-1400 | 1 | 0 | 0.03529 |
| 0,0 | 1 | 1 | 1 |
| 1400,1400 | 0 | 0 | 1 |
| 2400,2400 | 0 | 0 | 0 |

The API returns zero on failure, so zero samples alone do not prove unpainted
pixels. Overlapping nonzero layer values plus alpha-blend configuration mean a
normalized RGB weighted sum should not be assumed equivalent to this material.
Blend order/compositing and zero-coverage behavior need explicit verification.

Landscape transform: translation (-2800,-2800,0), scale (100,100,100), no rotation.
World bounds centre (0,0,0), half extents (2800,2800,100): 56 by 56 metres in XY.
Converted mesh `/Game/Meshes/Landscape/Test/SM_Landscape_Test` has local bounds
centre (0,0,12.109375), half extents (2800,2800,12.109375); actor translation is
(0,0,-30), unit scale, no rotation. These confirm matching horizontal coverage,
not pixel registration or geometric equivalence. Source: live transforms/bounds.

## Export gap and candidate next step

[Current JSON/exporter research](current-export-research.md) confirms the JSON
contains the mesh/material reference and DEBUG scalar but no painted weights,
mask transform or nine surface texture references. Assigning this material to
the static mesh does not itself carry Landscape paint weights onto that mesh.
Internal weightmap textures/allocation properties were not exposed through the
queried Python LandscapeComponent properties; no internal channel layout was recovered.

The live API documents `Landscape.render_weightmaps(world_transform, extents,
weightmap_layer_names, out_render_target)`. Its docstring says it renders final
weights for a requested top-down window; a 2D render target packs requested layers
into RGBA in supplied order. Zero-size extents request the entire loaded landscape.
The owner subsequently approved testing this seam with a temporary render target.
The test returned true and produced a nonempty 512 x 512 RGBA8 PNG:
[diagnostic mask](diagnostic-weightmap-dunes-rough-cliff.png).
Follow-up: the exported PNG has RGB weights but zero alpha at the inspected
pixels, making it invisible in normal image viewers. Use the
[opaque diagnostic preview](diagnostic-weightmap-dunes-rough-cliff-preview.png)
for viewing. This separate 24-bit RGB PNG discards alpha; every pixel's RGB
was compared against the original, with zero mismatches, and preview opacity
was verified as 255 throughout. The original bake is retained unchanged.
The requested window used identity world transform and XY extents
(-2800,-2800) to (2800,2800). Requested channel order was Dunes, Rough, Cliff
(RGB). After three frame yields, RenderingLibrary.export_render_target wrote
the image under this research directory. Visual inspection shows three soft
painted regions, black outside coverage and mixed colors including white where
all layers overlap. These are separate layer masks, not precomposited effective
surface contributions. No source assets or level were saved. The temporary
render target GPU resource was released after export.

Limit: the PNG is diagnostic. Exact linear numeric preservation through PNG
export, pixel-to-world orientation/centres and runtime DDS conversion were not
verified. An attempted force_linear_gamma property query was unavailable in
this Python binding. Do not use the preview as a finalized runtime mask contract.

Next planning work: establish a linear mask bake/export with an explicit chosen
channel order, world origin/extent/orientation and sampling convention; preserve
or deliberately change alpha compositing; verify a known landmark against the
converted mesh. Surface world projection/noise remain requested AGP behavior,
not existing features of this connected Unreal graph. A reference capture,
production mask export and end-to-end visual alignment remain pending. The
diagnostic channel order above is explicit and tested; internal Landscape
weightmap packing remains uninspected.

Primary live evidence is also logged in the project's `Saved/Logs/` Python log
entries for this session (generated logs may rotate and are not versioned).
