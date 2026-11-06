# Normal Landscape pipeline implementation handoff

Updated 2026-11-06. **Stopped at the user's request. Do not resume implementation
until asked.** This records unfinished work, not completed runtime acceptance.

## Goal in plain language

The user manually creates and paints a Landscape in Unreal, exports/reimports
its shape as a static mesh, then runs that scene through normal AGP. They do not
want a PowerShell preparation command, separate inspection scene, replacement
camera or changed lighting.

The new authoring step is to add TGA Export Settings to the runtime mesh actor
and choose its Source Landscape. Normal level export then packages paint and
material connections. AGP loads the ordinary scene JSON via initialScene.

## Workspace and authorization

- Engine: `C:/Users/tarik.bergstrom/Documents/GitHub/AGP`, branch `SplatmapV2`.
- Content: `C:/Users/tarik.bergstrom/Perforce/AGP-Content`, branch `ExporterUpdateBranch`.
- Baselines when this implementation started: engine
  `a0c5a7a827b3f98a2ab3df04fc69e873699d5a95`; content
  `dc7863d607b5fae116e82e7615a68dd4850ab75f`.
- User authorized implementation and parallel subagents. No staging, commit,
  push or Perforce submission is authorized or was performed.
- Both repositories already contain extensive edits/deletions from earlier
  cleanup, plus unrelated shader edits. Preserve them. In particular, do not
  overwrite changes in MaterialUtilities.hlsli or Normal.hlsli.
- Perforce is reachable when escalated. Use explicit clients `tarik_content` and
  `tarik.bergstrom_TGA-SP85_Mainline_884`. Existing pending adds/deletes/edits must
  be preserved. SceneAsset.cpp was opened for the scoped diagnostic logging edit.
- All three subagents completed; no agent work remains running. The final
  automation process completed with exit code 0. Unreal and Game were not left
  running by this implementation session.

Read root AGENTS.md, CODING_STANDARDS.md, formatter/editor configuration, and
Docs/ProjectVersionControl.md before continuing. Read the content repo's own
AGENTS.md and shared working rules before its edits.

## User's manual assets

- Map: `/Game/Levels/Test_Levels/SP/Lvl_Landscape_Manual`.
- Native Landscape: 2x2 components, one subsection, 63 quads, scale100; sculpted
  and painted by the user. Geometry alignment was manually confirmed.
- Material instance: `/Game/Levels/Test_Levels/SP/MI_Landscape_Manual`, parent
  `/Game/Materials/MM_Landscape`; three non-weight-blended LayerInfo assets.
- Static mesh: `/Game/Meshes/Landscape/Manual/SM_Landscape_Manual`.
- Runtime FBX: `AGP-Content/Content/Meshes/Landscape/Manual/SM_Landscape_Manual.FBX`.
- Original normal export:
  `AGP-Content/Content/ExportedScenes/lvl_landscape_manual/Lvl_Landscape_Manual_Level.json`.
- The user completed the Unreal FBX roundtrip, precision settings,
  ExportAssets.bat, final runtime FBX export and ordinary scene export.
- One component has only Dunes/Rough allocated; the other three also have Cliff.
  The implementation handles this by treating missing named layers as zero.
- **No Source Landscape link was saved into the user's map.** Acceptance sets
  that link transiently and never saves the map. Authoring and runtime-FBX hashes
  were checked before/after both automation runs and remained unchanged.
- No authored camera exists. Normal AGP supplies its debug camera. The original
  interactive JSON had an editor-only LandscapeGizmoActiveActor0 that disappears
  after editor reload; the saved map exports six authored actors.

## Implementation completed

Engine changes:

- `Source/Engine/GameFramework/UnrealSceneImporter/UnrealSceneImporter.cpp`:
  accepts Landscape TypeID12 as authoring-only metadata; resolves top-level
  material instances and parent ancestry independently of order; applies parent
  then child/component overrides; retains root shader masters; diagnoses
  duplicate names, missing parents, cycles and parse errors; defaults omitted
  transforms to identity with the existing coordinate conversion.
- `UnrealSceneStructs.h`: Landscape enum value12.
- `Source/Engine/GameFramework/AssetHandling/SceneAsset.cpp`: logs diagnostic
  context/message when normal scene loading fails.
- Existing untracked `Tests/GameFramework/NativeMaterialImportTests.cpp` was
  extended to13 cases. Its existing vcxproj remains the harness.
- `.gitignore` adds `/.scratch/normal-landscape/` for local evidence/builds.
- `Docs/LandscapeToAGP.md` now describes normal export/load and the source picker.
  Review its final field wording once documentation is finalized.

Content plugin changes under
`Unreal/P5G4_KnightHeart/Plugins/TGAUnrealPlugin/Source/TGAUnrealPlugin/`:

- `Public/TGAExportSettingsComponent.h`: SourceLandscape picker,
  SurfaceRepeatMeters=8, LandscapeNoiseStrength=0,
  LandscapeNoisePatchSizeCm=200.
- `Private/TGALandscapeExport.h/.cpp` (new): capture exact named source weights;
  canonical atlas channels Dunes/Rough/Cliff/unused0; rectangular assembly,
  missing layers, border/grid/transform validation, texel-centre world mapping,
  linear independent-channel DDS mip chain and generated material parameters.
- `Private/TGAUnrealPlugin.cpp`: collect real material ancestry; create a
  per-Landscape material instance; override only linked static-mesh slots;
  preserve ordinary scene data. Stage combined atlases/JSON, back up replaced
  files and roll back publication errors. Check JSON serialization success.
- `Private/TGALevelSerializer.cpp`: export authored Landscape materials rather
  than transient compiler material instances.
- `Private/Tests/TGALandscapeExportTests.cpp` (new): synthetic NamedAtlas test
  and generic opt-in CurrentScene normal-export acceptance.

Initial scope is the current three-snow-layer MM_Landscape profile, an
axis-aligned positive-scale single Landscape proxy, rectangular component grid,
one subsection and63 quads/component. It does not generate terrain geometry or
translate arbitrary Unreal material graphs. Linked actors sharing one source
must agree on surface/noise settings. Source DEBUG is inherited. Raw component
DDS masks keep their original allocations; combined atlas uses canonical ones.

## Built and verified

1. Full `Game.sln` Debug x64/v145 build: passed,7 existing warnings,0 errors.
2. NativeMaterialImportTests:13 cases passed, including malformed input,
   inheritance/overrides, Type12 and ordinary camera/light/custom preservation.
3. Existing real DirectX MaterialImportTests: passed.
4. Unreal final plugin build: passed. Final DLL/PDB/modules were installed in
   the actual content project's plugin and verified against the build hashes.
5. Final Unreal automation run: all3 passed:
   `TGA.LandscapeExport.CurrentScene`, `TGA.LandscapeExport.NamedAtlas`,
   `TGA.WeightmapExport.DirectDDS`.
6. CurrentScene uses the public normal exporter, the user's saved map, a
   transient source link, deliberate changed paint plus an invalid later link,
   checks that failed export preserves the previous runtime JSON/atlas, restores
   transient source data, and successfully re-exports. No authored assets saved.
7. Independent comparison of first fresh export:6 authored actors preserved,
   one narrowly identified transient editor gizmo omitted; all127x127 canonical
   mask samples exact; bindings/selectors/world mapping correct;7 DDS mips.
8. Production importer loaded the first fresh normal export as3 root masters
   and6 actors. The final fresh-export02 has not yet been independently compared
   or passed to the importer executable; only its full Unreal automation passed.

Initial UAT BuildPlugin failed in XGE with an internal uninitialized packet
error. Local build succeeded with per-invocation `-NoXGE -NoUBA`; no global
build settings changed. Direct-project engine builds need an explicit
SolutionDir; full Game.sln supplies it. Existing Unreal serializer property
initialization errors/warnings occur at startup before tests; the test results
above are explicitly Success.

## Evidence and commands

Local evidence is in engine `.scratch/normal-landscape/` (ignored by Git):

- `contract.md`, `verification.md`, `verify_normal_export.py`.
- `game-build.log`, `importer-test-build.log`, `importer-tests.log`.
- `material-test-build.log`, `material-tests.log`.
- `plugin-local-build.log`: final successful build, including serialization guard.
- `exporter-tests-final.log`: final3 successful tests.
- `fresh-export-01/` and `fresh-export-02/`: normal scene JSON/raw masks/atlas.
- `authored-hashes.json`: preserved user map/MI/LayerInfo/mesh/FBX hashes.
- `previous-plugin/`: installed plugin binary rollback copy.
- `install-and-test.ps1`: source/build matching, installation, automation and
  unchanged-authoring checks; accepts fresh output/log names.
- `plugin-build-01/HostProject/Plugins/TGAUnrealPlugin/`: isolated built plugin.

To independently verify the final export (read-only):

```powershell
& 'C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/ThirdParty/Python3/Win64/python.exe' .scratch/normal-landscape/verify_normal_export.py C:/Users/tarik.bergstrom/Perforce/AGP-Content/Content/ExportedScenes/lvl_landscape_manual/Lvl_Landscape_Manual_Level.json .scratch/normal-landscape/fresh-export-02/Lvl_Landscape_Manual_Level.json SM_Landscape_Manual
& ./Bin/Tests/Debug/NativeMaterialImportTests.exe .scratch/normal-landscape/fresh-export-02/Lvl_Landscape_Manual_Level.json
```

## Remaining work when resumed

1. Run the final fresh-export02 read-only comparisons above. Review scoped diffs
   and whitespace checks without touching unrelated work.
2. Deploy only the reviewed ordinary normal scene/atlas and required existing
   terrain assets to AGP, preserving destination differences. No such deployment
   was done during this implementation session. Startup still selects blockout:
   `current.application.initialScene = lvl_blockout/Lvl_Blockout_Level.json`.
3. Launch normal Game.exe with the ordinary manual scene, observe actual terrain
   rendering/material compilation and normal camera behavior. This has NOT been
   done. Build/import success does not establish visual acceptance.
4. Complete manual paint registration, seam, slope-normal/ORM,8m repeat,
   floaters, repaint and sculpt/re-export checks with the user.
5. Publish the pending content guide from engine
   `.scratch/normal-landscape/LandscapeExport.md` to
   `AGP-Content/Tools/Terrain/LandscapeExport.md`; update the content README.
   That destination file does not exist yet.
6. Retire duplicated legacy preparation route only after checking references:
   `Tools/Terrain/PreparePaintedLandscapeRuntime.ps1`, its `.md`, and
   `Tools/Tests/PaintedLandscapeRuntime.Tests.ps1` still exist. The old mask-only
   shader `M_PaintedLandscapeWeights.hlsli` remains; reconsider it once the legacy
   route is removed. Do not remove an active dependency blindly.
7. Update `Docs/SplatmapIntegration.md` from its earlier cleanup checkpoint to
   this normal pipeline implementation and actual validation results. It has
   not yet been updated for this implementation.
8. Reopen Unreal with MCP only when resuming. The user explicitly requested MCP:

   ```powershell
   Start-Process -FilePath 'C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList '"C:/Users/tarik.bergstrom/Perforce/AGP-Content/Unreal/P5G4_KnightHeart/P5G4_KnightHeart.uproject"','-PLUGIN="C:/Users/tarik.bergstrom/.unreal-mcp/0.6.1/Plugins/ModelContextProtocol/ModelContextProtocol.uplugin"','-EnablePlugins=ModelContextProtocol,PythonScriptPlugin'
   ```

   Guide the user to add TGA Export Settings to SM_Landscape_Manual, select its
   native Landscape, save and run normal Export Level to JSON. The user asked
   for one manual step at a time and plain explanations. Do not recreate an
   inspection scene or silently save a source link into the map.

## Known limitations/review notes

- Independent reviewer found the late-validation atlas overwrite bug; it was
  fixed and its regression passed.
- Nonblocking robustness note remains: staging cleanup always deletes backup
  files even if rollback itself encounters an I/O error. Consider retaining the
  staging folder and logging its path when restoration fails.
- User's Basic scene references Unreal-only SM_SkySphere/M_SimpleSkyDome absent
  from runtime assets. AGP can substitute a fallback cube. This predates the
  Landscape integration; do not silently remove the actor or claim sky parity.
- The retired inspection pipeline's exact Unreal/AGP shading parity was never
  signed off. Current integration has no new visual parity claim.
- No new product decisions currently require clarification. If one arises,
  user requested `$grill-me`: read repo `.agents/skills/grill-me/SKILL.md`, then
  `.agents/skills/grilling/SKILL.md`; ask about user-facing choices in plain
  language, not facts discoverable from code.

Earlier cleanup archives are outside both repositories at
`C:/Users/tarik.bergstrom/Documents/AGP-Cleanup-Archive-20261106`. Preserve them.
