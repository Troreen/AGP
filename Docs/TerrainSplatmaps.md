# Terrain splat material

Implementation is available on `splatmap-blending`, with game shader, runtime
assets and authoring tools in AGP-Content. The prepared comparison scenes render
correctly according to the owner's check. **The splatmap feature is not complete:**
the path from authoring a Landscape to using it in AGP is not yet human friendly.
The owner must complete the manual acceptance check below before we call it working.

## Continuation on the school computer

Start with [the current school handoff](SplatmapSchoolHandoff.md). It supersedes
older setup/status notes below and records the commands and remaining checks.

2026-10-23 handoff: use AGP branch `splatmap-blending` and AGP-Content branch
`main`. The latest work adds RGB noise comparisons, comments explaining the
compiled shader offsets, a content-sync batch launcher and prepared-terrain
material export through the normal TGA scene exporter.
Matching current content commit: `60f00b5bf67ee107f141dfa3a2c351298a9fdf37`.
Material inheritance and paint refresh were checkpointed at `4a3fb55`.
The earlier prepared-export handoff used `8cd98950fa1805c68e7be88a854dcb560435c771`.

The exporter update and its four Unreal automation checks passed in Unreal
5.7.4, including a normal scene export from transient test assets. The tested
plugin was installed locally; generated plugin DLLs and the comparison EXEs
are ignored and must be built/prepared again on another computer. See
AGP-Content's `Docs/TerrainSceneExport.md` for setup and verification details.

**Current continuation:** material-instance export now reads effective Unreal
parameters, including inherited defaults and child overrides. Those values win
over the separate prepared export settings; absent parameters retain the old
settings as fallbacks. Mask world bounds remain export metadata. The explicit
**TGA -> Expose Selected Terrain Master Texture Parameters** action converts
the audited master's nine connected texture objects into instance parameters,
preserving their defaults and connections. It does not save assets. Close the
master's editor and reload it first if the command reports a cached graph.

The open project's loaded plugin is still the earlier version. Close Unreal
normally, rebuild its plugin, and reopen to use these changes; verification
uses an isolated plugin project. See the
[active continuation](../.scratch/terrain-splatmaps/continuation-2026-10-23.md).

**Paint updates:** **TGA -> Refresh Selected Landscape Paint Mask PNG** now
refreshes an existing mask's source using saved bounds and named RGB layers.
Reimport the texture and prepare/deploy its DDS explicitly. Fresh-Landscape GPU
tests verify channels, orientation, repaint and output preservation.
**TGA -> Create Selected Landscape Paint Mask** now creates/imports the first
512 x 512 mask and records its bounds on the assigned material instance. Save
the new assets manually. DDS preparation supports matching custom source stems;
see AGP-Content's artist guide.

**Mesh updates:** the explicit **TGA -> Reimport Selected Terrain Mesh with
Corrected UVs** action prepares the raw FBX and updates the same mesh through
its saved yaw -90 pipeline. Keep raw input separate from generated prepared
copies. Runtime FBX export still needs Force Front X; assets are saved manually.

**Pick up next:** rebuild the plugin at school and perform the owner's fresh
Landscape test, including another sculpt/paint update and final normal appearance.
Do not start with more shader features or scratch-folder cleanup.

### Required owner acceptance check

Use the [manual functionality walkthrough](TerrainSplatmapFunctionalityTest.md)
to perform the run and record blockers/results stage by stage.

The owner explicitly requires a manual run from zero. A prepared test scene,
passing tests or a successful build cannot replace this check:

1. Start with a new Landscape in Unreal, rather than the prepared splatmap fixture.
2. Sculpt it, assign the terrain material/instance, set up its textures and paint
   recognizable regions for the supported layers.
3. Export/reimport through Unreal into the StaticMesh used in a scene, following
   the team's existing workflow. Make the UV convention and import settings
   predictable without requiring the artist to diagnose the FBX pipeline.
4. Use the normal TGA exporter to produce the scene and its required material
   data; make mask preparation/deployment understandable and reproducible.
5. Open that scene through the normal AGP launch and see the sculpted terrain
   with the assigned textures and painted layer placement working correctly.
6. Change the original Landscape's sculpting/painting, repeat the normal update
   path, and confirm AGP reflects those changes without replacing scene actors
   or constructing a special verification scene by hand.

The owner performs the authoring steps and judges whether they are usable.
Document the steps and observed result. Until this passes, describe the status
as "rendering demonstrated; authoring/export workflow incomplete."

Preserve the team's authoring workflow: sculpt/paint a Landscape, export/reimport
it through Unreal into an existing StaticMesh, place/use that mesh in scenes,
then export through TGA. Automatic UV correction and mask refresh at the
Landscape conversion step are still pending. Do not remove the intentional
import yaw -90 or introduce a global UV flip in AGP.

At school, inspect both working trees and the explicit Perforce clients before
updating files; see [version control](ProjectVersionControl.md). Update the
matching Git branches, download LFS assets, rebuild the Unreal plugin and AGP,
and preview content deployment before copying. No Perforce submission is
authorized by this handoff.

To recreate the comparison launches from the AGP root after syncing/building:

```powershell
foreach ($variant in 'Baseline', 'Noise', 'Weights', 'NoiseWeights') {
    powershell.exe -NoProfile -ExecutionPolicy Bypass -File .scratch/terrain-splatmaps/prepare-runtime-check.ps1 -Variant $variant
}
```

`Weights` and `NoiseWeights` are the RGB baseline/noise pair. `NoiseWeights`
generates its local scene from the committed RGB baseline; the generated scene
does not need copying out of this machine. No scratch-folder cleanup has been
performed; consolidate still-useful evidence before removing old investigations.

## Material contract

`MM_Landscape` is opaque and lit. Dunes is the continuous base; independent
Rough and Cliff masks are ordered alpha overlays. Final contributions are
`((1-r)*(1-c), r*(1-c), c)`. The raw Dunes mask is retained for diagnostics.
Color, linear AO/roughness/metallic and decoded normals use those contributions.

| Exported parameter | Meaning |
| --- | --- |
| `DunesBaseColor`, `DunesORM`, `DunesNormal` | Dunes surface textures, t4–t6 |
| `RoughBaseColor`, `RoughORM`, `RoughNormal` | Rough surface textures, t7–t9 |
| `CliffBaseColor`, `CliffORM`, `CliffNormal` | Cliff surface textures, t10–t12 |
| `SplatMask` | Linear RGB weights Dunes/Rough/Cliff, t13 |
| `BlendNoise` | Linear red-channel transition noise, t14 |
| `MaskU`, `MaskV` | Float4 affine rows applied to `(world.z, world.x, 0, 1)` |
| `SurfaceMapping` | Float4 reciprocal repeat XY and shared phase XY |
| `NoiseControls` | Float4 strength [0,1], patch size in centimetres, two reserved values |
| `DEBUG` | 0 surface; 1 composed layer colors; 2 raw mask; 3 encoded world normal; 4 ORM |

Unreal XY is AGP ZX. Surface repeats are 800 centimetres, with native Landscape
phase at Unreal (-2800,-2800). Mask coverage is independent and must contain
the entire authored level. Preparation checks transformed imported mesh bounds;
the clamp sampler is only border filtering, not permission to leave coverage.
The shader uses the geometric normal and shared world axes rather than mesh UV
tangents. Planar projection stretches steep faces; the near-vertical fallback
keeps normal output finite but does not provide triplanar quality.

Noise samples `world.zx / patchSize`; signed red values perturb only partial
weights through `w + (2*n-1)*strength*w*(1-w)`. Zero strength is the baseline;
zero and one weights remain exact. Terrain and floaters share the same phase.

## Repeatable preparation

All game-owned files below live in AGP-Content. Use new output paths and review
before replacing generated files. Do not run broad asset conversion as inspection.

1. `Tools/Terrain/CorrectLandscapeFbx.py` makes a new raw FBX with V precompensated
   in surface and weightmap UV arrays. Run using Blender's bundled Python and
   supply its `io_scene_fbx` module directory. The parsed comparison must allow
   only those UV changes and the writer's declared FileId/CreationTime changes.
2. Import that corrected FBX into Unreal with the intentional **yaw -90**. Use a
   separate mesh for verification; recompute normals and Mikk tangents. Do not
   change source textures' green channel to compensate for a UV convention.
3. In Unreal Python, load `Tools/Terrain/ExportTerrainRuntime.py` with `runpy` and
   call `export_terrain(landscape, corrected_mesh, new_output_folder, bounds)`.
   This schedules three editor ticks, exports named weights and the corrected
   StaticMesh, and writes mapping metadata. No source assets or levels are saved.
   **Force Front X must be true** for runtime FBX export; the default front axis
   changes geometry orientation relative to scene transforms in TGAFBX.
4. `PrepareTerrainTextures.ps1` converts the exported mask and noise source to
   lossless RGBA8 UNORM DDS with full mip chains. It verifies top-level RGBA in
   DDS and PNG round trips, preserves sources, and refuses existing outputs
   unless `-Force` is supplied. Keep originals for reproducibility.
5. `BuildTerrainScene.py` creates a distinct runtime scene using the existing
   exported parameter schema. Supply actual imported AGP geometry bounds; it
   rejects any terrain/floater outside the mask. Its guide documents controls.
6. Preview scoped content copying, copy the reviewed files into AGP, then build
   the game to deploy the shader. Engine suffix fallback still supports existing
   materials; explicit parameter roles take priority. See
   [material bindings](UnrealAGPMaterialNaming.md).

The supplied prepared FBX retains all geometry and model properties. Runtime
FBX was exported from the corrected temporary Unreal mesh with Front X enabled.
All 3249 merged TGAFBX vertices match original terrain positions to 9.1e-13;
UV0/UV1 are checked separately. The original Unreal level remains dirty as it was
before this work; it was not saved by the implementation.

## Owner visual check

The isolated launch copies under `Intermediate/TerrainVerificationRuntime` and
its noise/debug siblings use copied settings. Normal `Bin/Settings` is preserved.
These folders are generated verification artifacts, not production assets.

1. Run the baseline executable, then the noise-on executable from the same camera.
   The level should show terrain and two elevated 8m inspection floaters. Check
   gradual transitions, pure regions and unpainted Dunes areas. Noise should
   reshape transitions while leaving pure regions unchanged.
2. Use the layer-color version to compare asymmetric painted landmarks against
   Unreal. Red/green/blue show final Dunes/Rough/Cliff contributions; white in the
   original raw mask is not the expected ordered composite color.
3. Check texture placement across terrain and both floaters. One floater is
   rotated 45 degrees. Shared world coordinates should preserve texture and
   mask placement. Floaters are elevated for inspection; this fixture does not
   prove contact or height matching.
4. Check near/far and oblique views, then use F5/F6 renderer views for world
   normals, roughness, metallic and AO. The older local-normal view samples the
   standard normal texture before the material hook and is not this shader's
   final projected normal. Dunes and Rough base colors are identical assets, so
   use roughness and normal detail to distinguish their assignments.
5. Confirm directional normal detail and geometric slopes under lighting. A flat
   normal must retain the slope. Compare the rotated floater as well as terrain.
6. Record hardware/resolution and comparable GPU timings for baseline/noise.
   Automated WARP readback is not a hardware performance measurement.

Close and restart the chosen isolated executable after changing generated scene
parameters. Do not assume F4 reload invalidates all named material asset caches.

## Verification evidence

Debug x64 solution build passed. Material-import tests on RTX 4060 cover named
roles, legacy fallback, instance inheritance, compiled offsets, hidden padding,
oversized-buffer rejection, prepared scene parsing and the real material compile.
All four shader entry points compile with warnings treated as errors. The
standalone terrain GPU suite passes 19 fixed readbacks for color/ORM ordering,
noise endpoints/controls, world mapping and projected normals. Both linear DDS
conversions preserve every top-level RGBA byte and have complete mip chains.

Independent spec review found no confirmed code defect. Standards review found
stale binding documentation and a missing export-artifact success check; both
were corrected. Authored visuals, final draw bindings/filtering and hardware
timing remain pending. See the [feature workflow](../.scratch/terrain-splatmaps/workflow.md)
for exact commands, source pointers, continuation and remaining checks.
