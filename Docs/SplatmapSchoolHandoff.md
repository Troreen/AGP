# Splatmap school handoff

Updated 2026-10-23. Start here when continuing on the school computer.

The AGP renderer blends the three surface layers and noisy transitions. Unreal
authoring commands now expose texture parameters, create/import the first paint
mask and refresh it after painting. Strict UV preparation and same-mesh reimport
are implemented and automatically checked. **The feature still needs Tarik's complete
fresh-Landscape walkthrough and visual acceptance in AGP.**

## Repositories and saved state

| Repository | Branch | Verified checkpoint before this handoff |
| --- | --- | --- |
| AGP | `splatmap-blending` | `0091d15` |
| AGP-Content | `main` | `60f00b5` |

This handoff and the final mesh-reimport changes are committed after those
checkpoints; see the final commit IDs in the task response / Git history.
No Perforce submission is authorized. Check remote availability before leaving
home: local commits do not transfer to school until pushed.

## School setup

1. Locate both actual checkout roots. Inspect Git status and explicit Perforce
   client mappings/opened files before updating. Preserve school/team changes;
   follow [project version control](ProjectVersionControl.md). Do not force sync,
   reset or broadly reconcile the workspace.
2. Update AGP `splatmap-blending` and AGP-Content `main` through the normal Git
   workflow; download content LFS assets. The tools and plugin source live in
   AGP-Content, not AGP's ignored runtime `Content/` copy.
3. Close Unreal normally before rebuilding its plugin. The home editor was
   left open with an older DLL; it was never closed/saved by the agent. Generated
   plugin binaries and isolated test projects are ignored and must be rebuilt.
4. Build the Unreal 5.7 project plugin with the installed compatible toolchain:

   ```powershell
   & "$ueRoot/Engine/Build/BatchFiles/Build.bat" UnrealEditor Win64 Development "-Project=$contentRoot/Unreal/P5G4_KnightHeart/P5G4_KnightHeart.uproject" -WaitMutex -NoHotReloadFromIDE
   ```

   Set `$ueRoot` and `$contentRoot` to this machine's real paths. Reopen Unreal.
   UV preparation uses Blender's bundled Python/FBX modules; Blender 5.1 was
   tested. The command discovers a compatible standard Windows installation
   beneath Program Files/Blender Foundation; no home paths are saved in settings.
5. Preview scoped content deployment with `SyncContent.ps1 -WhatIf`, copy the
   reviewed runtime assets, then build AGP `Game.sln`, Debug x64, VS v145.
   A build deploys shaders; copying content alone does not. See [workflow](AgentWorkflow.md).

## Artist workflow to verify

Use a new level and Landscape, preserving existing team materials/maps.

1. Reuse `MM_Landscape` and create an instance. If the master still hardwires
   textures, select it in the Content Browser and run **TGA -> Expose Selected
   Terrain Master Texture Parameters** once. Close/reload its editor if a cached
   graph is reported. Save only when ready. Set Dunes/Rough/Cliff color, ORM and
   normal textures in the normal instance editor; effective inherited values
   now reach AGP without duplicate export fields.
2. Sculpt and paint recognizable asymmetric regions using `Snow_Dunes`,
   `Snow_Rough`, `Snow_Cliff` Layer Info assets with independent alpha blending.
3. Follow the team's raw Landscape FBX conversion/import route. On the existing
   terrain StaticMesh, preserve the required import yaw -90. Use **TGA -> Reimport
   Selected Terrain Mesh with Corrected UVs** and select the original raw FBX.
   The command prepares a new corrected copy and reimports the same mesh through
   its stored pipeline. Do not select a prepared copy or delete its provenance.
   Generated copies live under project `Saved/TerrainPrepared`; regenerate them
   on another computer. Review and save the mesh manually. Repeat this command
   after a new raw sculpt export, continuing to use the same scene mesh/actor.
4. Assign the same instance to Landscape and placed terrain mesh. Select the
   native Landscape and run **Create Selected Landscape Paint Mask**. Choose a
   new source PNG in AGP-Content. It creates/imports a linear 512-square mask and
   records world bounds on the instance. Load the whole covered, unrotated native
   Landscape first. Save the texture and instance normally.
5. Set linear transition noise with **Edit Selected Terrain Material Export
   Data**. For later paint edits use **Refresh Selected Landscape Paint Mask PNG**,
   then explicitly reimport the texture and prepare/deploy its DDS.
6. Run `Tools/Terrain/PrepareTerrainTextures.ps1` with `-MaskName` and `-NoiseName`
   matching the source PNG stems. Old defaults remain supported; custom names
   allow ASCII letters, digits, underscores and hyphens. Use `-Force` only for
   intentional prepared-output updates. See AGP-Content `Docs/TerrainSceneExport.md`.
7. Export corrected runtime FBX separately with **Force Front X enabled**. Keep
   the original raw FBX in an authoring/source folder. Use the mesh's expected
   runtime filename and avoid duplicate stale FBXs with the same asset name.
   Normal TGA scene JSON export does not itself extract/deploy all mesh assets.
8. Export the scene through normal TGA, deploy reviewed FBX/DDS/shader assets,
   and launch that scene in ordinary AGP. Check sculpt shape, painted placement,
   material detail, noise off/on and another sculpt/paint update.

Record results in [the functionality test](TerrainSplatmapFunctionalityTest.md).
The user declined an added normal scene-export coverage check; do not add it back.

## Verification and continuation

- AGP Debug x64 solution build and material-import integration tests passed.
- Fifteen integrated Unreal terrain/material/mask GPU checks passed with zero
  test warnings/failures on RTX 3070 / DirectX 11 / Unreal 5.7.4.
- The isolated real-mesh experiment retained all 4096 positions, 6272 triangles,
  slots, yaw and UObject identity; UV correction error was at most `5.96e-08`.
  A repeated raw-source operation did not double-flip UVs.
- Final production mesh-reimport checks and review findings are recorded in
  [the continuation](../.scratch/terrain-splatmaps/continuation-2026-10-23.md)
  and [ticket 07](../.scratch/terrain-splatmaps/issues/07-terrain-mesh-reimport.md).
- The final combined run passed 19 production checks plus the isolated reimport
  experiment, zero test warnings/failures. It includes a real sculpt update and
  renamed source node. Two Python provenance-guard tests pass too.

The owner approved including the saved `MM_Landscape` and `Lvl_Splatmap_Testing`
asset edits in this checkpoint. These live editor edits were not saved by the
agent or substituted for the required new-Landscape acceptance test.

Final owner visuals, tangent/normal appearance, runtime Front-X export after the
new command, hardware timings and the complete from-zero acceptance remain
pending. Preserve old investigation evidence; do not spend the next session on
scratch cleanup or extra shader features before the walkthrough.
