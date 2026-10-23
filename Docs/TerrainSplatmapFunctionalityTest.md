# Manual terrain splatmap functionality test

Status: **not passed**. Rendering has been demonstrated with prepared fixtures;
the complete authoring/export workflow is still unfinished.

## Goal

Tarik starts with a new Landscape in Unreal, sculpts it, assigns textures,
paints its layers, exports/reimports a mesh through Unreal, exports the scene
through TGA, and sees the intended result in **AGP**. He can then change the
Landscape and repeat the process without assistance or hand-editing scene JSON.

This is a walkthrough and an acceptance test. Where a step is not supported
yet, record the blocker and fix the workflow before resuming. Do not replace
the new terrain with the existing splatmap test assets and call the test passed.
Technical workarounds may help diagnose a problem, but do not count as a usable
artist workflow unless the owner accepts and can repeat the documented steps.

## Before starting

- [ ] On the school computer, inspect both Git working trees and the actual
  Perforce clients before updating files. Follow [version control](ProjectVersionControl.md).
- [ ] Update AGP branch `splatmap-blending` and AGP-Content branch `main`.
- [ ] Download LFS files in AGP-Content with `git lfs pull`.
- [ ] Build the updated TGA plugin for the installed Unreal 5.7 editor, then
  restart Unreal. The home computer's generated plugin DLL is not in Git.
- [ ] Confirm **TGA -> Edit Selected Terrain Material Export Data** is visible.
- [ ] Build AGP `Game.sln`, **Debug | x64**, with the documented VS toolchain.
- [ ] Choose a new test level and asset folder in AGP-Content. Keep existing
  team levels and materials intact. Reusing the supported master material and
  texture library is allowed; reusing prepared terrain/mask fixtures is not.

Record:

| Item | Value |
| --- | --- |
| Tester / date | |
| AGP commit / branch | |
| AGP-Content commit / branch | |
| Unreal version | |
| New level / asset folder | |
| Export destination | |
| GPU / resolution | |

## 1. Create an unmistakable new Landscape

1. Create a new test level and save it in the chosen folder.
2. Enter Landscape mode and create a small Landscape suitable for inspection.
3. Sculpt an asymmetric shape: one raised hill near a corner, a shallow trough
   near another edge, and a flat inspection area. Avoid a symmetric shape that
   could hide a rotation or reflection.
4. Save the level. Record its position, scale and size.

Expected: the Landscape is editable and its shape is easy to recognize from an
oblique view. Capture an Unreal reference image showing the hill and trough.

- [ ] Pass
- Blocker / confusing step:

## 2. Set up a material instance and paint

1. Create a new material instance based on the supported `MM_Landscape` master.
2. Assign it to the new Landscape.
3. Set up the supported Dunes, Rough and Cliff texture sets: each has base
   color, packed AO/roughness/metallic and normal textures.
4. In Landscape Paint mode, create/assign Layer Info assets for the material's
   target layers. Preserve the supported independent alpha-layer blend setup;
   do not silently switch to normalized weight blending. The current layer
   identifiers are `Snow_Dunes`, `Snow_Rough` and `Snow_Cliff`.
5. Paint a broad Rough stripe, a separate Cliff island, and an overlapping
   transition. Leave an unpainted area to check the Dunes base behavior in AGP.
6. Save the material, Layer Info assets and level, then capture the pattern.

Expected: the owner can understand which textures belong to each layer and
which material-instance values will actually reach AGP. The painted pattern is
asymmetric enough to detect an incorrect mask orientation or position.

**Current blocker:** the existing master has hardwired textures. The provisional
export settings duplicate those choices and override matching normal material
parameters. Making normal instance parameters the artist-facing source of
truth is the first continuation task. Do not mark this step passed merely
because export metadata can be manually filled with a second set of values.

- [ ] Pass
- Blocker / confusing step:

## 3. Export/reimport the Landscape as the scene mesh

1. Export the new Landscape to FBX using the team's normal Unreal workflow.
2. Import it as a StaticMesh using the established settings for that conversion.
3. Keep the native Landscape as the editable source. Place the converted mesh
   in the test scene, using a saved material instance for its runtime surface.
4. Compare its hill, trough, scale and placement against the source. Check a
   directional/checker texture as well; the world-space splat shader alone
   cannot prove that mesh UVs are correct.
5. Record the FBX path, mesh asset and import settings for the later reimport.

Expected: geometry orientation, size, UV orientation and texture phase are
predictable. A reimport should update the same mesh asset without replacing
the scene actor. Preserve the intentional yaw -90 for the existing tested
conversion; do not guess a new rotation just to hide a UV reflection.

**Current blocker:** automatic V correction at this handoff is not implemented.
`AGP-Content/Tools/Terrain/CorrectLandscapeFbx.py` is an existing technical
preparation tool, not an integrated artist command. Decide/document the usable
conversion step before expecting an unassisted pass here.

- [ ] Pass
- Blocker / confusing step:

## 4. Prepare this Landscape's paint data

1. Produce a fresh mask from this Landscape's named painted layers, with
   Dunes/Rough/Cliff in RGB. Use this terrain's actual world coverage.
2. Prepare the mask and transition-noise runtime DDSs as linear numeric data,
   with the required mipmaps. Keep base-color, ORM and normal conversions on
   their established color-space/format paths.
3. Associate the mask, its coverage, the surface origin/repeat size and noise
   controls with the material used by the placed mesh.
4. Confirm the mask covers the terrain and any additional meshes using it.

Expected: the owner can refresh the paint data without calculating shader
mapping vectors or editing JSON. The mask must come from the newly painted
Landscape, not the committed museum/test mask.

**Current blocker:** TGA serializes prepared inputs; it does not bake the mask
or refresh it after painting. Existing technical helpers are
`Tools/Terrain/ExportTerrainRuntime.py` and `PrepareTerrainTextures.ps1` in
AGP-Content. Their current assumptions and manual handoffs still need a usable
workflow. A nonempty exported PNG alone does not establish correct channels,
orientation or coverage.

- [ ] Pass
- Blocker / confusing step:

## 5. Export the actual scene through TGA

1. Use the normal TGA **Export Level to JSON** command on the test scene.
2. Save the export under AGP-Content's `Content/ExportedScenes/`, following the
   normal level-folder naming convention. Record the generated relative path.
3. Check the export's result message. Missing inputs should identify the
   affected actor/material rather than silently producing an incomplete scene.
4. Ensure the runtime FBX/DDS assets referenced by the export are present in
   AGP-Content's `Content/`. TGA scene serialization does not itself deploy them.

Expected: the placed mesh retains its transform/material slot and the exported
instance references `MM_Landscape` with the correct textures, mask and controls.
No test-scene generator or manual JSON parameter patch is needed.

Provisional setup details are in AGP-Content's `Docs/TerrainSceneExport.md`.
Use its **TGA -> Edit Selected Terrain Material Export Data** dialog only with
the limitations recorded above; it is not evidence that normal instance
inheritance is finished.

- [ ] Pass
- Blocker / confusing step:

## 6. Run the scene in ordinary AGP

1. Review content-copy differences with `AGP-Content/SyncContent.bat -WhatIf`.
   Preserve any desired AGP-side asset edits, then copy the reviewed content.
   At school, follow the verified Perforce workspace procedure before writing
   imported content; use `SyncContent.ps1 -EngineRoot <actual-path>` if needed.
2. Build AGP after copying so its deployed material shaders are current.
3. Record the existing `current.application.initialScene` in
   `Bin/Settings/ApplicationSettings.json`. Set it to this new export's path
   relative to `Content/ExportedScenes`, using the normal scene-selection setup.
4. Launch from Visual Studio or `Bin/Debug/Game.exe`, rather than a copied
   terrain verification executable. Restart between material-setting changes.
5. Use the debug camera if needed: F1 toggles it; hold right mouse to look,
   WASD to move and Space/Ctrl to move vertically.

Check:

- [ ] The new hill/trough, orientation, scale and placement match Unreal.
- [ ] Rough stripe, Cliff island and overlapping regions appear in the right places.
- [ ] Unpainted terrain uses Dunes; no missing-texture or black holes appear.
- [ ] Color, roughness and normal detail change with the intended layers.
- [ ] Texture repeat size and phase are correct at near/far and oblique views.

Use F5/F6 renderer views for world normals, roughness, metallic and AO. Dunes
and Rough currently share identical base-color assets, so color alone cannot
verify that their ORM and normals are assigned correctly.

- [ ] Pass
- Screenshot paths / errors / confusing step:

## 7. Compare transitions and instance overrides

1. Through the intended material controls, select RGB layer contributions and
   export with noise strength zero. Run the new scene in normal AGP.
2. Record the camera view. Set noise strength to 0.75, re-export/deploy and
   restart AGP with the same view.
3. Check red Dunes, green Rough and blue Cliff transitions: noise should change
   partial blends while fully painted and absent layers remain unchanged.
4. Create a child instance and change one layer texture or artist control
   through normal instance overrides. Assign it to an inspection mesh and
   confirm export/AGP uses the override while retaining other inherited values.

Expected: material-instance inheritance is understandable and no competing
export-metadata value silently wins. This is currently a known limitation.

- [ ] Pass
- Screenshot paths / errors / confusing step:

## 8. Make another sculpt/paint iteration

1. Return to the original editable Landscape. Raise the hill and move part of
   the Rough stripe so both geometry and paint changes are obvious.
2. Repeat the documented export/reimport into the **same** mesh asset.
3. Refresh its mask through the documented workflow, export the scene through
   TGA, deploy and restart ordinary AGP.
4. Confirm both changes reached AGP. Actor placement and material assignment
   must survive; old paint or geometry must not linger silently.

- [ ] Pass
- Extra manual work / stale output / confusing step:

## Result and continuation

- [ ] All stages completed manually by the owner, without substituting prepared fixtures.
- [ ] The owner agrees the documented workflow is usable for the team.
- [ ] Repeated sculpt/paint changes reached AGP correctly.
- [ ] Existing non-terrain materials still export and render normally.

Overall result: **Not run / Blocked / Failed / Passed**

First failing stage:

Exact action and observed result:

Expected result:

Next change needed:

Restore the previous startup scene if desired. Save source assets and required
runtime outputs in AGP-Content before any future sync or device switch. A pass
here does not by itself establish collision/navigation support or GPU performance.

## Background reading

Epic's [Landscape quick start](https://dev.epicgames.com/documentation/en-us/unreal-engine/landscape-quick-start-guide-in-unreal-engine?application_version=5.7)
covers creation, sculpting and painting. Its [Paint Mode guide](https://dev.epicgames.com/documentation/en-us/unreal-engine/landscape-paint-mode-in-unreal-engine?application_version=5.7)
explains target layers and Layer Info assets. The AGP-specific conversion,
runtime formats and completion criteria are defined in this repository's
[terrain guide](TerrainSplatmaps.md), not by those general Unreal tutorials.
