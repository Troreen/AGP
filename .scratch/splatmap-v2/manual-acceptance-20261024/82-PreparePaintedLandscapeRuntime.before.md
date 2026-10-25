# Prepare the painted Landscape FBX inspection scenes

For the full hands-on export, deployment, Blender handoff, runtime comparison,
and signoff sequence, see [AGP manual acceptance](../../../AGP/Docs/SplatmapV2ManualAcceptance.md).

Use this after a normal TGA export of Lvl_Splatmap_Testing2 containing both the
native painted Landscape and the reimported Lvl_Splatmap_Testing2 mesh. Keep the
native Landscape for authoring. The tool does not modify Unreal assets, repair
FBX geometry, edit the normal scene export, deploy files or change startup settings.

From the AGP-Content root, prepare the textured and lit RGB-base-color scenes:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File Tools/Terrain/PreparePaintedLandscapeRuntime.ps1 -SourceScene Content/ExportedScenes/lvl_splatmap_testing2/Lvl_Splatmap_Testing2_Level.json -OutputDirectory Content/ExportedScenes/lvl_splatmap_testing2_runtime -TerrainMeshName Lvl_Splatmap_Testing2 -MaterialMode Surface -DirectionalLightIntensity 2 -Force
```

`-Force` permits regenerating this output. Surface mode binds the existing
MM_Landscape shader with all nine layer maps and the transition-noise DDS. Noise
strength is zero for the baseline comparison. Layer1 uses Snow_Dunes as the
continuous base, Layer2 uses Snow_Rough as an overlay, and Layer3 uses Snow_Cliff
as the final overlay. Layer2/3 mask selectors come from their exported metadata,
independent of RGB channel order. A fully weighted upper layer replaces lower
layers. Surface tiling and projected normal logic are preserved.

`-SurfaceRepeatMeters` sets the world-space surface texture repeat for prepared
Surface scenes; the default is `8`. For example, pass `-SurfaceRepeatMeters 16`
to repeat every 16 metres. The tool derives both `SurfaceMapping` scale and
phase from that one value and the existing -2800 cm surface origin, then writes
the mapping into the scene material. The mask mapping remains separate. Scene
parameters override the shader's named fallback `SurfaceRepeatMeters` value;
change the preparation option when regenerating these scenes.

`-DirectionalLightIntensity 2` reproduces the current local lighting adjustment.
Without this option, exported light intensities are retained. The default
`-MaterialMode Weights` is a separate unlit mask-only preparation; use Surface
for the current RGB-base-color view with normals and ORM active.

## Input and mask mapping

Fresh PNGs are preferred when present; otherwise preparation reads the lossless
linear RGBA DDS files produced by ExportAssets.bat. After repainting, run normal
TGA export before preparing. After sculpting, also repeat the geometry export and
runtime FBX preparation. An old mesh does not update when only masks are exported.

Supported input is bounded: one axis-aligned native Landscape with positive
scale; one-subsection components with 63 quads (override with -ComponentQuads);
one packed RGBA mask per component; identical layer/channel allocation across
components; exactly one named runtime FBX and mesh component. Unsupported
transforms, differing allocations, holes and unequal shared-border samples fail
rather than produce a guessed result. Repainting may change channel allocations;
inspect any reported mismatch instead of assuming a global RGB order.

The current four 64x64 masks form a 127x127 sample grid. Shared rows/columns are
written once and must agree exactly in RGBA. World mapping uses texel centres.
There is no UV flip, channel normalization or layer reordering. Unreal world XY
maps to AGP world ZX. For this export: red=Snow_Rough, green=Snow_Dunes,
blue=Snow_Cliff. Alpha remains intact in the DDS but is not displayed as a colour.

## Scene outputs and viewing

The separate prepared scenes retain other source actors, use the FBX terrain,
omit native authoring Landscape records and use a top-down inspection camera.
Source cameras are replaced only in these inspection copies. Unused incompatible
material records are removed. Output includes two scene JSONs, the combined mask
DDS, an opaque white DDS and preparation.json with hashes and mapping controls.
The white input prevents independent mask alpha from hiding an unlit mesh.

Choose current.application.initialScene in AGP/Bin/Settings/ApplicationSettings.json:

- `lvl_splatmap_testing2_runtime/Lvl_Splatmap_Testing2_Runtime_Level.json`:
  full Landscape surface with textured base color (DEBUG=0).
- `lvl_splatmap_testing2_runtime/Lvl_Splatmap_Testing2_Runtime_RGB_Level.json`:
  the same lit material with raw RGB weights replacing only base color (DEBUG=2).
  Layer normals, roughness, metalness and AO remain active and unchanged.

Review content-copy differences before deployment. Build to deploy shader sources,
then fully restart Bin/Debug/Game.exe to release cached meshes/materials. Compare
mask regions against Unreal using the cube landmarks, including corners, shared
edges, slopes, scale and orientation. F5/F6 cycle renderer diagnostics. Local Space
Normals shows the final material normal in the mesh tangent frame; neutral tangent
normals naturally appear purple. A build/import check alone is not visual acceptance.

## Current FBX correction and remaining work

AGP's prebuilt FBX importer introduces node translation into the original terrain
normals. The current runtime FBX has its node transform baked into positions and
its direction vectors rotated/normalized. Its geometry and UVs are identical to
the original. The Unreal authoring/export FBX is preserved. The user confirmed
that corrected runtime lighting works. See the source/candidate comparisons in
.scratch/splatmap-v2/validation/normal-light-diagnosis/diagnosis.md.

The graphics team uses Blender for the engine-ready runtime FBX handoff. Work on
a copy of each newly exported Unreal FBX; preserve the Unreal authoring FBX.
Import the copy in Blender, inspect the mesh and its object/parent transforms,
axis orientation, dimensions, UV layers, face orientation and custom normals.
Apply the intended object transform into mesh data on the runtime copy so the
exported FBX has the same world-space shape and placement with an identity mesh
node transform. Blender's Object > Apply > Location, Rotation and Scale transfers
object transforms into mesh data while keeping its displayed placement. Check
parents and FBX axis/unit conversion too; applying the visible object alone may
not remove every exported node transform. Export only the corrected terrain mesh
as FBX to a separate output file and reimport it for inspection before handoff.
Keep the original UV registration and preserve custom normals when they are
valid. Recalculate Outside is not a general correction for an open Landscape
surface or custom split normals; use face orientation and normal displays to
diagnose, then verify the final normals through AGP's actual FBX importer.
Blender references: [Apply transforms](https://docs.blender.org/manual/en/5.1/scene_layout/object/editing/apply.html),
[FBX import/export](https://docs.blender.org/manual/en/5.1/files/import_export/fbx.html),
and [editing normals](https://docs.blender.org/manual/en/5.1/modeling/meshes/editing/mesh/normals.html).

Compare the Blender output with the new source export: mesh bounds/placement,
vertex positions in the engine coordinate system, UV0/UV1, painted-mask coverage,
normal lengths and face/normal alignment. Sculpting changes positions, so compare
against that sculpt's source FBX, not the older 16,129-vertex baseline. A Blender
viewport match alone does not validate the AGP import. The previous handoff
proved the manual route for the earlier mesh; it does not validate a new sculpt.

Run ExportAssets.bat before installing the corrected runtime FBX, because its
FBX mirroring can overwrite the correction with the Unreal authoring export.
Then place the reviewed Blender FBX at Content/Meshes/Landscape/Test/
Lvl_Splatmap_Testing2.fbx, run preparation, and ensure preparation.json records
the corrected runtime FBX hash. The tool writes MeshSHA256 automatically; record
the authoring FBX hash and Blender correction details alongside it after each
regeneration (the current AuthoringFbxSHA256/RuntimeFbxPreparation fields were
added manually). Preview the
content copy, deploy the corrected FBX and prepared scenes, and fully restart AGP
to release cached mesh/material instances. If ExportAssets.bat runs again, repeat
the final corrected-FBX copy and hash check. Do not reuse the old corrected mesh
after sculpting. A fresh repaint/export/prepare/deploy/restart cycle, a fresh
geometry/export/Blender-correction/deploy cycle, and visual comparison with Unreal
remain open acceptance work.

## Verification

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File Tools/Tests/PaintedLandscapeRuntime.Tests.ps1
```

The current validation folders record exact atlas bytes/placement, texel-centre
mapping, source preservation, invalid-input guards, texture bindings and rendered
comparisons. In RGB-base-color mode, 1,865 albedo pixels change while zero world
normal, roughness, metalness or AO pixels change. The actual renderer probe source
is saved under .scratch/splatmap-v2/validation/rgb-surface-01. Reproduce it from
AGP/.scratch/splatmap-v2 as described by the validation notes. No alternate
renderer is used. The original unlit shader remains available for Weights mode.
