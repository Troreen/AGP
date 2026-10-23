# Splatmap authoring continuation, 2026-10-23

Status: in-progress. Rendering demonstrated; complete authoring/export workflow
still awaits the owner's from-zero acceptance test.

## Request and starting point

Owner asked to continue development and, at 10% remaining, write a handoff and
commit both repositories. Account usage percentage is not exposed by available
tools; maintain this document as a checkpoint. No new push or Perforce submit
authorization. Starting trees were clean:

- AGP `splatmap-blending`, `09f3e1771941a3151324ce222c36aec2efd71f52`.
- AGP-Content `main`, `8cd98950fa1805c68e7be88a854dcb560435c771`.

Verified content checkpoint committed as
`4a3fb550e74b32b6f10ca0157d5fc068ffae4fda` on `main`. This AGP handoff commit
records the matching continuation state. Neither repository was pushed.

## Current slice

Resolved ticket [04](issues/04-material-instance-export.md) makes normal material-instance
parameters authoritative for terrain texture roles, `SurfaceMapping`,
`NoiseControls` and `DEBUG`. Unreal resolves parent/default values and child
overrides. Existing export fields remain fallbacks when a parameter is absent.
`MaskU/MaskV` always come from the prepared mask's metadata bounds.

The source master has nine connected hardwired TextureObjects. An explicit
**TGA -> Expose Selected Terrain Master Texture Parameters** command converts those
audited `T_8M_Snow_*` defaults into nine connected parameters. No automatic
conversion during export, and no source asset save. Conversion is undoable,
idempotent, and rejects ambiguous/partial/disconnected graphs before changes.
Close the master editor and reload the asset first if its graph is cached.

## Verification so far

- Existing plugin regression test reproduced the bug: separate `DEBUG=1`
  replaced a child's `4` and an inherited parent's `2`.
- Fixed plugin compiled in Unreal 5.7.4; all seven serializer/normal scene
  export tests passed. Local reports: `Intermediate/TerrainInstanceRed` and
  `Intermediate/TerrainInstanceGreen`.
- All ten `TGA.TerrainExport` checks and the separate authored-master integration
  check pass. Final local report: `Intermediate/TerrainFinalInstanceChecks`.
  The authored check used isolated copies of `MM_Landscape`, `MF_LandscapeLayer`
  and nine snow texture assets, converted a temporary duplicate, and verified
  original function connections, defaults, DEBUG and source preservation.
- Independent spec review found an explicit-null child override hidden by
  Unreal's normal getter. A failing regression reproduced it; resolution now
  checks the nearest explicit instance entry, and the reviewer confirmed the fix.
  Standards review found no confirmed violation; a connection-record suggestion
  was classified optional and deferred.
- Existing AGP MaterialImportTests built Debug x64 and passed on RTX 3070.
  The sandbox's first MSBuild invocation failed in Visual Studio FileTracker;
  running with the necessary access succeeded. Normal Game.sln Debug x64 build
  also passed (`Intermediate/TerrainContinuationSolutionBuild.log`).

## Paint-refresh slice

Ticket [05](issues/05-mask-refresh.md): explicit selected-Landscape mask PNG
refresh, keeping source resolution, named RGB order and saved mask bounds. No
mesh edits, source package saves, automatic reimport, DDS conversion or deployment.
Both real-GPU `TGA.TerrainMask` checks passed offscreen on RTX 3070 using D3D11
(`Intermediate/TerrainMaskGpuFirst`). They import new asymmetric independent
paint, verify exact RGB/orientation/dimensions, repaint at the same path, and
verify heights stay unchanged. Invalid coverage, rotation, a missing boundary
component and locked destination preserve old PNG bytes. Both independent
reviews found no confirmed defect. The final integrated GPU run passed all
thirteen checks with zero test warnings/failures:
`Intermediate/TerrainContinuationCheckpoint/index.json` (Unreal 5.7.4,
DirectX 11 SM5, RTX 3070). Earlier synthetic fixtures produced harmless shader
warnings from an invalid test-only texture-object mask; that fixture was fixed
and the clean run above includes the correction.

This is a usable refresh step, not an initial mask creator: the material must
already reference an imported mask with an existing writable PNG source. Native
Landscape rotation is conservatively rejected (StaticMesh import yaw is separate).
Initial mask bootstrap and integrated UV correction remain next. The owner's
from-zero acceptance test is still pending.

An Unreal editor is open and holds the real plugin DLL. The first in-place
build compiled successfully but failed to link because of that lock. Nothing
closed or saved the editor. Verification uses the isolated project:

`Intermediate/TerrainExporterInstanceCheck/HostProject/HostProject.uproject`

The real project's plugin source is updated, but its loaded DLL is still old.
Close Unreal normally, rebuild the project plugin, then reopen before using
new commands. Do not copy a DLL over a loaded plugin.

## Build and checks

From AGP, build the isolated host with:

```powershell
& 'C:/Program Files/Epic Games/UE_5.7/Engine/Build/BatchFiles/Build.bat' UnrealEditor Win64 Development "-Project=$PWD/Intermediate/TerrainExporterInstanceCheck/HostProject/HostProject.uproject" -WaitMutex -NoHotReloadFromIDE
```

Run `UnrealEditor-Cmd.exe` against that project with `-unattended -nop4 -nosplash
-nullrhi`, `-ExecCmds="Automation RunTests TGA.TerrainExport;Quit"`,
`-TestExit="Automation Test Queue Empty"`, a unique `-ReportExportPath` and
`-abslog`. Redirect console output to a local log. Judge individual test results
and the final automation exit code, not unrelated pre-existing plugin startup
warnings about uninitialized JSON reflected structs.

## Next work

Concrete API pointers and investigated implementation constraints are recorded
in [authoring next steps](authoring-next-steps.md).

1. Add an initial-mask creation/import command. The refresh now works through
   TGA once a mask exists, but cannot bootstrap a freshly painted Landscape.
   Existing `Tools/Terrain/ExportTerrainRuntime.py` can create an initial mask
   through technical preparation, coupled to an already corrected StaticMesh.
2. Make DDS preparation/reimport/deployment understandable and repeatable. The
   current PowerShell converter still publishes fixed mask/noise names; custom
   source filenames need matching runtime names before claiming general support.
3. Integrate mesh conversion UV correction with the team's existing workflow;
   keep the intentional import yaw -90 and reject a global AGP UV flip.
4. Walk through [fresh Landscape acceptance](../../Docs/TerrainSplatmapFunctionalityTest.md)
   with the owner. Prepared test scenes and automation are not this acceptance.

Do not add shader features or clean old investigation files ahead of these gaps.
Keep authored assets and unrelated settings unchanged. Plugin binaries and
isolated test outputs are ignored and must be rebuilt on another device.
