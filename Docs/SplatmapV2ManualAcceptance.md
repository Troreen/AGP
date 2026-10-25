# Splatmap V2 manual acceptance

This guide records the final hands-on checks for the painted Landscape export
and AGP runtime pipeline. The automated baseline and current implementation
status are in [the home continuation](SplatmapV2Continuation.md); exact
preparation limits and Blender handoff guidance are in the AGP-Content guide
[`Tools/Terrain/PreparePaintedLandscapeRuntime.md`](../../AGP-Content/Tools/Terrain/PreparePaintedLandscapeRuntime.md).
The content repo's authoritative tickets are
`.scratch/splatmap-v2/issues/02-painted-export.md` and
`03-runtime-pipeline.md`.

## Before starting

Use Unreal Engine 5.7 and Blender 5.1.2. Work in a saved copy of
`/Game/Levels/Test_Levels/SP/Lvl_Splatmap_Testing2`; keep the native painted
Landscape and its Unreal authoring FBX intact. Make one intentional paint change
in an easily recognized area, and later one intentional geometry change on a
separate pass. Do not make unrelated art changes. Save the map and record the
starting and changed landmarks with screenshots.

The latest implementation verification passed a fresh `Game.sln` Debug x64
v145 build, all eight terrain shader compilations with `fxc /WX`, the
ContentScripts suite, the extended PaintedLandscapeRuntime suite, and freshly
built MaterialImportTests. The preparation suite checks ordinary material
defaults and TwoSided, actor components with empty and non-empty behavior
payloads, and skylight cubemap rotation in both Weights and Surface outputs.
MaterialImportTests now checks texture parameter-role matching before suffix
fallback. A fresh v145 DirectX probe passed for textured DEBUG=0 and lit RGB
DEBUG=2: 15 actors, two masters, 16,129 terrain vertices, face/normal alignment
0.999565, ten surface/noise bindings, and 8 m mapping
`[0.00125, 0.00125, 3.5, 3.5]`. RGB mode changed 1,865 albedo pixels while
leaving world normals, AO, roughness, and metalness unchanged. Runtime artifact
hashes and the two built shaders/helpers match their deployed copies. Current
probe/build evidence is under `AGP/Intermediate/SplatmapV2Verification`; the
Game.sln build log is `AGP/.scratch/splatmap-v2/game-build-current.log`.

`GameFrameworkTests` could not build because its existing source refers to the
removed `SetInputMapper` API and the project is missing bundled importer source;
this is outside the splatmap change and is not counted as passing. Preparation
preservation checks do not prove that Unreal serialized a fresh scene correctly
or exercise multiple Landscape proxies. These checks establish a baseline; they
do not replace the fresh editor export and visual checks below.

## 1. Export the changed paint from Unreal

1. Open the saved test-map copy in Unreal 5.7. Confirm the native Landscape,
   the reimported `Lvl_Splatmap_Testing2` terrain mesh, the cube landmarks, and
   the other scene actors are present. Confirm the rebuilt TGA exporter loads.
2. Inspect the painted layer allocation on every Landscape component. Record
   the layer name-to-channel mapping; do not assume red, green, or blue has a
   fixed meaning after repainting.
3. Export the map through the plugin's normal **TGA** scene-export command to
   `AGP-Content/Content/ExportedScenes/lvl_splatmap_testing2/`. Do not prepare
   from an older export. Preserve the generated JSON, per-component weightmap
   PNGs, FBX, and exporter log as this run's evidence.
4. Check the JSON component records and filenames. Confirm every mask is
   present, dimensions and channels agree with the saved Unreal allocation,
   component/proxy names make output filenames unique, and adjacent shared
   borders agree. Compare mask regions against the visible paint and landmarks.
5. For the exporter preservation check, use a separate disposable test-map
   fixture containing a second Landscape proxy/component sharing a weightmap
   and ordinary non-Landscape actor/material, camera, and behavior component
   (including an empty custom payload if available). Confirm the exporter
   preserves these records and distinct proxies do not overwrite one another's
   mask files. Keep this exporter fixture separate from runtime preparation:
   the current preparer deliberately accepts one native Landscape actor and
   does not claim multi-proxy runtime support. Do not infer the exporter check
   from the current single-proxy runtime probe.

**Pass evidence:** map copy and changed paint screenshots; export command/log;
JSON and mask output paths; channel-allocation notes; border and filename
inspection; explicit multi-proxy/preservation-fixture result. Any lost actor,
missing mask, collision, or unexplained channel change is a failure to resolve.

## 2. Prepare both runtime inspection scenes

From the AGP-Content repository root, run the established asset conversion and
then prepare from the fresh normal TGA export:

```powershell
.\ExportAssets.bat
```

For a paint-only pass with unchanged geometry, restore the recorded corrected
runtime FBX after `ExportAssets.bat` and before running the preparation command:

```powershell
$candidate = '.scratch/splatmap-v2/validation/normal-light-diagnosis/Lvl_Splatmap_Testing2_baked_candidate.fbx'
$runtimeMesh = 'Content/Meshes/Landscape/Test/Lvl_Splatmap_Testing2.fbx'
Copy-Item -LiteralPath $candidate -Destination $runtimeMesh -Force
$runtimeHash = (Get-FileHash -LiteralPath $runtimeMesh -Algorithm SHA256).Hash
if ($runtimeHash -ne '922941C79F3DBB02EFDDF9040B9F0DE572270B2C884E3C968FC8CC22C46046D0') { throw 'Corrected runtime FBX hash mismatch.' }
```

For a new geometry export, use the newly corrected Blender FBX at the same
runtime mesh path instead of this older candidate. With the appropriate
corrected runtime FBX now in place, prepare the fresh export:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File Tools/Terrain/PreparePaintedLandscapeRuntime.ps1 -SourceScene Content/ExportedScenes/lvl_splatmap_testing2/Lvl_Splatmap_Testing2_Level.json -OutputDirectory Content/ExportedScenes/lvl_splatmap_testing2_runtime -TerrainMeshName Lvl_Splatmap_Testing2 -MaterialMode Surface -DirectionalLightIntensity 2 -Force
```

`ExportAssets.bat` can replace the corrected runtime FBX with its Unreal source
copy. Check its exit code. If geometry is unchanged, restore the previously
verified runtime correction after the batch step and before preparation; for
the recorded baseline, the candidate is
`.scratch/splatmap-v2/validation/normal-light-diagnosis/Lvl_Splatmap_Testing2_baked_candidate.fbx`
and its verified SHA256 is
`922941C79F3DBB02EFDDF9040B9F0DE572270B2C884E3C968FC8CC22C46046D0`. For new
geometry, create and verify a new Blender correction as described in step 4;
never reuse the earlier candidate for a new sculpt. The preparer writes
separate textured and lit RGB inspection scenes, a combined linear mask DDS, an
opaque white DDS, and `preparation.json`; it does not repair FBX transforms or
deploy content. `-Force` regenerates the named output folder.
The preparer supports one axis-aligned Landscape, one packed RGBA mask per
component, identical channel allocation on all components, a gap-free grid with
matching shared borders, and exactly one named runtime FBX/mesh component. It
must fail clearly if a fresh repaint falls outside those constraints; do not
reinterpret or hand-edit generated data to hide a failure.

Review the output metadata and hashes, mask mapping, and both scene JSONs. Then
preview deployment from AGP-Content (replace the EngineRoot if the checkouts are
not siblings):

```powershell
& .\SyncContent.ps1 -EngineRoot 'C:\path\to\AGP' -WhatIf
```

Inspect every reported conflict and changed destination. Resolve differences
intentionally, then copy the validated content; include
`-OverwriteChangedFiles` only when the reviewed destinations are meant to be
replaced. Destination-only files are retained. Build AGP so the current content
shader is deployed, and fully exit/restart `Bin/Debug/Game.exe` to release
cached meshes and materials.

## 3. Verify the fresh paint and runtime views

Select each scene in `AGP/Bin/Settings/ApplicationSettings.json`, run
`Bin/Debug/Game.exe`, and fully restart after changing the selection:

- `lvl_splatmap_testing2_runtime/Lvl_Splatmap_Testing2_Runtime_Level.json` —
  textured base colors, normals and ORM (DEBUG=0).
- `lvl_splatmap_testing2_runtime/Lvl_Splatmap_Testing2_Runtime_RGB_Level.json` —
  raw mask RGB displayed as base color while lit layer normals and ORM remain
  active (DEBUG=2).

At the same camera landmarks, compare the new painted region in Unreal and AGP:
corners, component seams, slopes, orientation, and cube placement. Confirm the
changed paint appears where expected and no old mask is being loaded. In the
material's `MB_DEBUG` parameter, inspect 1 for ordered layer contributions, 2
for raw RGB mask channels, 3 for encoded world normal, and 4 or greater for
packed ORM values. DEBUG=2 is a raw mask view; it is not the contribution view.
Use F5/F6 for renderer diagnostics; Local Space Normals should show a coherent
terrain normal field (neutral tangent normals can look purple).

Confirm Layer1 Snow_Dunes is the continuous base, Layer2 Snow_Rough overlays it,
and Layer3 Snow_Cliff overlays both. A fully painted upper layer must replace
the layers beneath it, not add brightness or create a normalized RGB blend.
Compare textured color, surface tiling, projected normals, AO, roughness, and
metalness. Save matching Unreal/AGP screenshots and note any visible mismatch.

## 4. Change geometry and make the corrected runtime FBX

1. Starting from the saved test copy, make one intentional geometry change and
   save it. Export a fresh normal TGA scene/FBX. Keep that Unreal authoring FBX
   unchanged as the source of truth.
2. Use **Force Front X Axis** for the native Landscape FBX export. Keep that
   convention through the runtime handoff: the verified Blender export uses
   **X Forward / Z Up** with the axis transform baked. The default -Y-front
   Unreal export produced a 90-degree native/runtime registration error in the
   2026-10-24 acceptance run. Verify native height-field correspondence after
   import; matching an already rotated FBX is insufficient.
   In Blender 5.1.2, import a copy of this new FBX. Inspect object and parent
   transforms, axis/unit conversion, dimensions, UV layers, face orientation,
   and custom normals. Apply the needed transforms into the runtime mesh data;
   applying the visible object transform alone may not remove parent or FBX
   node transforms. Preserve valid custom normals and UV registration. Do not
   use Recalculate Outside as a blanket fix for an open Landscape or custom
   split normals. Verify UV1 explicitly: Unreal's Landscape FBX can store
   `WeightmapUVs` as `LayerElementBumpUV`, which Blender 5.1.2's standard
   importer omits. Restore that source channel as an explicit second UV layer
   when present; do not accept a correction that loses its values. Export only
   the corrected terrain to a separate FBX, then
   reimport that output in Blender for inspection. The linked prep guide has
   the detailed Blender cautions and official Blender 5.1 references.
3. Run `ExportAssets.bat` before installing the corrected runtime FBX. Copy the
   reviewed Blender output to
   `AGP-Content/Content/Meshes/Landscape/Test/Lvl_Splatmap_Testing2.fbx` after
   the batch step; otherwise the batch may overwrite the correction. Run the
   same preparation command from step 2 again, inspect `preparation.json`, and
   record both the Unreal authoring FBX SHA256 and the corrected runtime mesh
   SHA256 plus the Blender correction note. Recheck the recorded runtime hash
   after any regeneration.
4. Preview and review content deployment again, copy the accepted files, build
   AGP, and fully restart the game. Never reuse the previous corrected mesh
   after changing geometry.

Compare the new Blender output with that same export and the actual AGP import:
world-space positions/bounds and placement, UV0/UV1 registration, painted-mask
coverage, normal lengths, face/normal alignment, and face orientation. Confirm
the geometry change is visible while the fresh paint remains aligned. Compare
textured and lit RGB scenes against Unreal at the same landmarks, including
normals and ORM. A Blender viewport match alone is not a pass; the imported AGP
result is the acceptance target.

## 5. Optional surface-repeat check

The normal setting is 8 metres. To compare a 16 metre surface repeat, regenerate
the prepared Surface scenes with `-SurfaceRepeatMeters 16` added to the same
preparation command. Deploy and restart as above. All nine layer surface maps
(base color, normal, and ORM for each of the three layers) use this repeat, so
their world-space detail scale changes together. The mask mapping, painted
weights, blend regions, and mesh registration stay fixed; transition-noise
mapping is controlled separately. Restore the 8 metre scene before final
signoff unless 16 metres is the intended saved value.

## Signoff record

Attach the evidence under `AGP-Content/.scratch/splatmap-v2/validation/` or
another agreed durable location. Record the actual commands, source/output
paths, source and runtime hashes, Unreal/Blender versions, screenshots, and
pass/fail for each gate. Keep tickets 02 and 03 open until all required gates
pass and the user confirms the final Unreal comparison.

| Gate | Owner | Result / evidence |
| --- | --- | --- |
| Fresh paint export, layer/channel allocation, mask borders and unique filenames | Graphics team | Pass: run log 23–25 and 33; native pixels and normal sculpt re-export verified |
| Multi-proxy/shared-weightmap and non-Landscape preservation fixture | Graphics team | Main checks pass: run log 36–37, eight masks and 14 ordinary actors; empty payload not exercised |
| Fresh preparation, reviewed deployment and visible repaint after restart | Graphics team | Pass: run log 23–25 and 33; scoped deployed hashes and actual runtime captures |
| New geometry export, Blender correction, hash/provenance and actual AGP import checks | Graphics team | Pass: run log 32–33; zero native height error, UV0/UV1 unchanged, new sculpt deployed |
| Textured/RGB/debug views and final Unreal visual parity | Graphics team | FAIL/open: matched captures differ; user authorized proceeding with failure retained; final signoff pending |
| Ground floaters sample the terrain mask with shared world mapping | Graphics team | FAIL: run log 39; all 12 cubes use imported Phong material, zero share the prepared ground material; fix and boundary/rotation runtime check pending |
| Optional 8 m / 16 m repeat comparison | Graphics team | Optional |
