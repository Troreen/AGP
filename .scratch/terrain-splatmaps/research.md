# Terrain splat maps and Unreal Landscape interchange

Research date: 2026-10-19. Scope: explanation and planning, not implementation. Sources are Epic's public documentation, retrieved on this date; unversioned pages currently identify themselves as Unreal Engine 5.8 documentation. The project's actual Unreal version and exporter must still be established.

## Start here

A terrain import usually needs two different kinds of information:

| Information | Beginner's interpretation | Intended use in AGP |
| --- | --- | --- |
| Landscape heightmap | How high is the ground at each grid point? | Build terrain geometry and answer ground-height queries. |
| Weightmap / terrain splat map | How much grass, rock, dirt, etc. appears at each point? | Mix surface materials. |
| Material detail height | Which tiny parts of rock or dirt protrude within a material? | Improve material transitions, if supported. |

Epic explicitly separates sculpted heightmaps from painted weightmaps. Paint controls material distribution. Therefore, ordinary material weights do **not** provide enough information to recover terrain elevation: a fully grassy pixel could lie on a valley floor or a mountain. This is an inference from the separate data roles, not an exporter-specific promise. [Landscape Paint Mode](https://dev.epicgames.com/documentation/en-us/unreal-engine/landscape-paint-mode-in-unreal-engine)

For this project, use “splat map” to mean surface blend control until an actual asset demonstrates another meaning. The word is not a reliable file specification: an Epic UEFN example calls a packed noise/slope/flow mask a splatmap. Ask for channel meanings, rather than interpreting colors by appearance. [Epic vista material example](https://dev.epicgames.com/documentation/fortnite/star-wars-vistas-and-biome-environments-in-fortnite?lang=en-US)

## Verified Unreal facts

### Terrain elevation

Unreal exports Landscape elevation as **16-bit grayscale PNG or R16**. Export options include row flipping, single versus tiled output for World Partition, selected edit layer versus the final blended edit-layer result, and loaded versus all regions. These choices affect what AGP receives. A PNG file alone does not record the Landscape actor's world transform. [Importing and Exporting Landscape Heightmaps](https://dev.epicgames.com/documentation/en-us/unreal-engine/importing-and-exporting-landscape-heightmaps-in-unreal-engine)

The documented local elevation range is approximately -256 to +255.992 before multiplication by Landscape Z scale; default scale 100 yields approximately -256 to +256 metres. Horizontal scale determines grid spacing. Landscape dimensions count vertices, not cells: 127 samples contain 126 cell intervals. Supported landscape resolutions depend on component layout and need not be powers of two. [Landscape Technical Guide](https://dev.epicgames.com/documentation/en-us/unreal-engine/landscape-technical-guide-in-unreal-engine)

**Candidate decoding to validate against an export fixture:** for unsigned 16-bit sample `s`, use `localZ = (s - 32768) / 128`. This is the evenly spaced signed mapping inferred from the documented range and precision. For an unrotated Landscape, `worldZ_cm = actorZ_cm + localZ * landscapeScaleZ`. At scale 100, samples 32768 and 32896 should differ by 100 cm; one sample step is 0.78125 cm. Apply the full actor transform for a rotated landscape, then the agreed Unreal-to-AGP coordinate/unit conversion. This formula should become an accepted import contract only after a known-height fixture verifies it. [Landscape Technical Guide](https://dev.epicgames.com/documentation/en-us/unreal-engine/landscape-technical-guide-in-unreal-engine)

Do not confuse a 16-bit PNG's decoded integer samples with a normalized color image or Unreal's internal component texture channels. The proposal here consumes exported grayscale files, not internal engine textures.

### Surface weights and blending

Unreal's native image data is single-channel 16-bit grayscale for heightmaps and single-channel 8-bit grayscale for target-layer masks. RGB input can discard color and limit precision. Its custom-format interfaces distinguish heightmap and weightmap formats. [Custom Heightmaps and Target Layers](https://dev.epicgames.com/documentation/en-us/unreal-engine/creating-and-using-custom-heightmaps-and-layers-in-unreal-engine)

Weight-blended paint layers compete: painting grass reduces the other weighted layers. Non-weight-blended layers are independent, useful for overlays such as snow. This painting configuration is distinct from the material graph's blending decisions. [Landscape Paint Mode](https://dev.epicgames.com/documentation/en-us/unreal-engine/landscape-paint-mode-in-unreal-engine)

Layer samples are weights in [0,1]. A simple weighted material computes `surface = sum(weight[i] * material[i])`; for example, 0.75 grass plus 0.25 rock. Unreal also supports ordered alpha blending. Its height blending modifies material transition weights using detail-height inputs; it does not mean the exported paint weights encode sculpted elevation. Multiple height-blended layers can produce zero-contribution black regions. Internally, Unreal packs target-layer weights into texture channels, adding a texture per four layers. Four channels are a packing convenience, not a universal four-layer terrain limit. Hardware/shader costs still matter. [Landscape Materials](https://dev.epicgames.com/documentation/en-us/unreal-engine/landscape-materials-in-unreal-engine)

## Recommended first import contract — proposal, not agreed scope

Begin with one small, untiled Landscape, final blended output, identity rotation, positive scale, and a limited set of weight-blended surface layers. Use separate height and material import milestones so either can be verified independently. Avoid promising arbitrary Unreal material-graph reproduction.

| Field | Proposed requirement |
| --- | --- |
| Elevation asset | One lossless 16-bit grayscale PNG; preserve integer precision through decoding. R16 is a later alternative with explicit byte order and dimensions. |
| Surface assets | One lossless 8-bit grayscale image per named layer, pending confirmation of the actual exporter. Packing into RGBA is an optional AGP processing step with an explicit channel-to-layer table. |
| Extent/alignment | Exact pixel dimensions, shared terrain extent, row origin and axis direction; require equal grid dimensions initially. Do not silently resize. |
| Placement | Source actor translation, rotation and scale; sample spacing; source units; agreed destination coordinate mapping. |
| Height meaning | Raw unsigned encoding, zero point, vertical scale and offset, explicitly versioned and verified using known elevations. |
| Surface meaning | Stable layer names, AGP material references, tiling settings, blend mode; no assumed meaning for R/G/B/A. |
| Weight policy | For weighted layers, normalize decoded nonnegative weights before mixing; choose a named fallback for zero total. Do not normalize independent overlays into this group. |
| Export provenance | Unreal version, tool/export settings, source landscape identifier, final versus selected edit layer, exported region. |
| Unsupported content | Reject or clearly identify holes, tiled worlds, arbitrary procedural materials and unsupported overlays until designed. |

These are proposed engineering choices. They are not claims that Unreal automatically exports this metadata, these separate weight files, or an AGP-ready package.

## Pitfalls the plan should address

- **Height is missing:** a colorful mask alone cannot establish ground elevation. Require its paired heightmap or already exported geometry.
- **Lost precision:** an 8-bit conversion collapses 65,536 possible height samples to 256. A general color loader must be inspected before using it for elevation.
- **Wrong world size:** use `(sampleCount - 1) * spacing`, then apply coordinate conversion exactly once.
- **Mirrored or displaced painting:** confirm orientation using an asymmetric fixture with visibly distinct corners. Height and weights must share the same transform.
- **Incorrect gamma:** treat heights and weights as numerical data, with no sRGB color transfer. This is a proposed importer requirement.
- **Unreal appearance mismatch:** painted masks do not capture arbitrary procedural material logic, texture UV choices, normals, roughness, or displacement. Define the intended visual subset explicitly.
- **Ground query mismatch:** rendering, sampling and collision should share height data and the same interpolation/triangle convention. Bilinear interpolation can disagree with a triangulated rendered cell.
- **Unbounded terrain cost:** a naive 4033 by 4033 grid would have 16,265,089 vertices and 32,514,048 triangles. Start small and decide scale/performance budgets before promising production landscape support.
- **Misreading documentation:** Epic's custom-layer guide contains a legacy-looking dimension table that conflicts with its prose instruction to match Overall Resolution. Use measured export dimensions and a fixture; do not encode that table as an importer rule.

## Unknowns to settle before implementation

1. Which Unreal version, Landscape authoring setup, material and export workflow are actually in use?
2. Can we obtain one tiny known-height Landscape export, its paint masks, and transform/export settings? No project asset was supplied for this research.
3. Does “height information” mean drawing terrain, a `heightAt` query, object placement, physics collision, or all of these?
4. How many layers, which blend semantics and what visual fidelity are required? Are materials painted, procedural, or mixed?
5. What terrain dimensions, world spacing, memory budget and camera distances matter? Are holes, streaming, tiles or LOD required initially?
6. What can the current AGP image/mesh/material/import path already do? A separate repository inspection should establish this; this note does not assert the engine has no support.

## Suggested evidence for a later implementation plan

Use a small asymmetric terrain with a flat plateau, ramp, known high/low points and sharply marked corner layers. Record expected positions and heights in source units. Verify decoded precision, extents, orientation and known sample values before rendering. Then compare the terrain silhouette and painted regions in a controlled AGP scene against Unreal captures. Add a height-query check only once its interpolation and out-of-bounds behavior are specified. Collision and large-world support require their own acceptance criteria if selected.
