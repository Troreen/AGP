# Splatmap V2 acceptance run — 2026-10-24

This running record starts when Unreal MCP was connected successfully. Follow
[the manual acceptance plan](SplatmapV2ManualAcceptance.md). Record each further
action, approval, result, artifact, and unresolved check here as work proceeds.
Times below use Europe/Stockholm (UTC+02:00); capture metadata also records UTC.

## Agreed scope and starting point

- The user wants narrated steps, permission before each execution step, and
  user involvement when a visual judgment is needed.
- The user authorized installing and configuring Unreal MCP, enabling local
  Python remote execution, and restarting Unreal. They confirmed there was no
  unsaved work to preserve during that restart.
- The user chose the existing test map
  `/Game/Levels/Test_Levels/SP/Lvl_Splatmap_Testing2` directly. Skip the plan's
  additional saved map copy for this run.
- The observed scene is the team's existing setup. Subsequent preparation and
  validation establish the new pipeline. The empty terrain Static Mesh slot is
  part of this starting state; filling it is a pending pipeline setup step.
- The user requested documenting every step, starting with the successful MCP
  connection. No paint or geometry edits, fresh exports, conversion, deployment,
  or runtime comparisons have happened in this run yet.
- Existing unrelated modifications in both repositories are preserved. Nothing
  has been staged, committed, pushed, or submitted to Perforce.

## 1. Connect and verify Unreal MCP — 20:21

**Authorization:** explicit approval to configure local-only remote execution,
register the MCP server in Codex, and restart Unreal.

**Installed server:** [BoldPhoenix/UnrealMCP](https://github.com/BoldPhoenix/UnrealMCP),
commit `602727c0b5732d6c6fa556a6e6c9913a6c5d5a96`, installed at
`C:/Users/tarik/Documents/UnrealMCP` in an isolated Python 3.14 environment.
`pip check` reported no broken requirements. Epic's `remote_execution.py` was
copied from the installed Unreal 5.7 engine into the bridge's vendor folder.

**Configuration applied:**

- Explicitly enable `PythonScriptPlugin` in the KnightHeart `.uproject`.
- Add Python settings in the project's `Config/DefaultEngine.ini`:
  `bRemoteExecution=True`, multicast endpoint `239.0.0.1:6766`, bind address
  `127.0.0.1`, TTL `0` (local machine).
- Register global Codex MCP server `unreal` with command
  `C:/Users/tarik/Documents/UnrealMCP/.venv/Scripts/python.exe -m unreal_mcp`
  and `UNREAL_PROJECT_DIR` pointing to the KnightHeart project.
- Back up the original project, Engine INI, and Codex configuration under
  `C:/Users/tarik/Documents/UnrealMCP/setup-backups/`.
- Close Unreal through its normal window-close request, confirm it exited,
  and relaunch Unreal 5.7 with the KnightHeart project.

**Verification:** a real MCP stdio client initialized successfully, listed 74
tools, and called `get_status`. Result: `isError=false`, world `Untitled`,
`ready=false`, actor count `0`. This establishes communication; a test map had
not yet been opened. The two project configuration files contain ten added
lines in total. No map content was edited during installation.

## 2. Open the existing test map — 20:22:54

**Authorization:** user approved inspection and then directed us to use the
existing test map without making another copy.

**Action:** MCP `open_level` with
`/Game/Levels/Test_Levels/SP/Lvl_Splatmap_Testing2`.

**Result:** `opened=true`, world `Lvl_Splatmap_Testing2`.

**Evidence:** [request](../.scratch/splatmap-v2/manual-acceptance-20261024/01-open-source.json),
[MCP result](../.scratch/splatmap-v2/manual-acceptance-20261024/01-open-source.result.json).

Starting on-disk map SHA256, captured before opening:
`17ABE74F3801F553D4E142E75BF8AC1B17AD2E0DA437C3CF78A0EFF95FBFB594`.

## 3. Inspect scene actors and exporter availability — 20:23:09

**Action:** read-only editor Python through MCP `execute_python`. Record engine
version, current world, actor transforms, mesh references, Landscape components,
and whether the TGA settings class can be loaded.

**Observed:**

- Engine `5.7.4-51494982+++UE5+Release-5.7`.
- Fifteen actors: one native Landscape, one directional light, twelve cube
  landmarks, and the terrain StaticMeshActor named `Lvl_Splatmap_Testing2`.
- Landscape location `(-6300, -6300, 0)` cm; scale `(100, 100, 100)`.
- Four Landscape components, numbered `0` through `3`.
- Terrain actor at `(0, 0, 0)`, unit scale, with a null Static Mesh reference.
- TGA plugin settings class available. This proves plugin class availability;
  fresh exporter behavior and the deployed binary's correspondence with the
  latest exporter source remain to be verified.

**Evidence:** [request](../.scratch/splatmap-v2/manual-acceptance-20261024/02-inspect-source.json),
[MCP result with actor inventory](../.scratch/splatmap-v2/manual-acceptance-20261024/02-inspect-source.result.json).

## 4. Inspect Landscape metadata and terrain asset availability — 20:23:38

**Action:** read-only MCP Python queries of Landscape component properties and
the asset registry; inspect the terrain files on disk.

**Observed:** component section origins `(0,0)`, `(63,0)`, `(0,63)`, `(63,63)`;
Landscape material `/Game/LevelPrototyping/Materials/MI_Landscape_SplatMapTesting`.
The attempted component size, weightmap allocation, weightmap texture, Landscape
GUID, and Landscape-layer property queries were unavailable through these
Python property names. No layer-to-channel mapping has been established for
this run. Obtain that evidence through an appropriate editor/exporter interface.

The asset-registry name search found the map but no imported StaticMesh asset
named `Lvl_Splatmap_Testing2`. Its Unreal source FBX exists at
`AGP-Content/Unreal/P5G4_KnightHeart/Content/Meshes/Landscape/Test/Lvl_Splatmap_Testing2.fbx`
(911,456 bytes). A separate runtime FBX exists at
`AGP-Content/Content/Meshes/Landscape/Test/Lvl_Splatmap_Testing2.fbx`
(921,120 bytes). Neither was modified. Source and runtime hashes remain to be
recorded before subsequent pipeline work.

The local TGA plugin DLL is 492,544 bytes, last modified 2026-10-23 02:27:07
local time. Timestamp/size alone do not establish exporter correctness.

**Evidence:** [request](../.scratch/splatmap-v2/manual-acceptance-20261024/03-terrain-details.json),
[MCP result](../.scratch/splatmap-v2/manual-acceptance-20261024/03-terrain-details.result.json).

## 5. Capture the starting Landscape overview — 20:23:56

**Action:** MCP `eyes_overview`, 1280 × 1280, padding `1.15`.

**Result:** capture returned successfully for `Lvl_Splatmap_Testing2`, 15 actors.
The image shows colored terrain and cube landmarks. Color appearance alone
does not identify packed paint channels or establish material parity. This is
an MCP scene render of the live editor world, rather than an editor-window
screenshot or evidence of saved state. User visual confirmation is pending.

**Evidence:** [image](../.scratch/splatmap-v2/manual-acceptance-20261024/04-baseline-overview-image-0.png),
[request](../.scratch/splatmap-v2/manual-acceptance-20261024/04-baseline-overview.json),
[capture metadata](../.scratch/splatmap-v2/manual-acceptance-20261024/04-baseline-overview.result.json).

## Current checkpoint

MCP is working and the existing test map is open. Inspection and the starting
overview are recorded. The user clarified that this is the team's baseline and
requested this running record from MCP connection onward.

The proposed reuse/import of the existing FBX is paused while the authoring
workflow is clarified. The user wants to validate how a team starts with a
painted Unreal Landscape, iterates on it, creates updated FBX geometry, and sees
the updated result in AGP. Inspect ForsakenCastle as an existing team example.

All required manual acceptance gates remain pending. The separate disposable
multi-proxy exporter fixture and geometry/Blender pass have not started.

## 6. Clarify FBX provenance and the intended iteration loop

**User clarification:** the existing FBX was created by exporting/reimporting in
Unreal. That route produced the previously investigated bugs. The team's starting
asset is an editable painted Unreal Landscape; repeatable export after editing
is part of the workflow being validated.

**Actions:** read the prior workflow, export inspection, normal diagnosis, and
terrain helper code; locate the ForsakenCastle map and hash both current FBXs.
No imports or authoring edits were made.

**Findings:**

- The historical workflow explicitly retained native Landscape painting with
  manual FBX export/reimport. TGA scene export and runtime preparation are
  separate steps; the preparer does not generate native Landscape geometry.
- The historical export inspection recorded an FBX-generated material parent
  (`FBXLegacyPhongSurfaceMaterial`) differing from the expected Landscape master.
- The normal diagnosis found unit-length directions in the original FBX, but
  invalid transformed normals after AGP's bundled FBX import. Node translation
  was included in normal transformation. The corrected runtime candidate baked
  node transforms while preserving positions and UVs. This distinguishes the
  observed downstream importer defect from an assumption that all source FBX
  normals were already invalid.
- Current Unreal authoring FBX SHA256:
  `2002A493239A8359FE9E0440C09D8493AEACF20D74513DF370F1A17BB43E5686`.
- Current corrected runtime FBX SHA256:
  `922941C79F3DBB02EFDDF9040B9F0DE572270B2C884E3C968FC8CC22C46046D0`.
  Both match the previously documented versions. These are historical geometry,
  not a new sculpt/export from this run.
- `Tools/Terrain/ExportTerrainRuntime.py` requires an already corrected mesh;
  it does not establish a general native-Landscape-to-runtime-FBX conversion.
- ForsakenCastle map found at
  `/Game/Levels/Game_Levels/Lvl_Forsaken_Castle`.

**Next inspection:** read ForsakenCastle actors and Landscape structure through
MCP. Check for dirty maps before switching, perform no saves, and return to the
original test map only if no dirty maps would be discarded. Record the result.

## 7. Inspect the ForsakenCastle parent map — 20:28:36

**Action:** execute the guarded read-only MCP inspection described above.

**Result:** no dirty map prevented inspection. The immediate parent-level
inventory returned 19 actors, including four Level Instances, lighting, cameras,
and gameplay actors. It returned no Landscape in that immediate inventory;
this does not establish the contents of asynchronously loaded child levels.
Returned to `Lvl_Splatmap_Testing2` successfully without saving either map.

**Evidence:** [request](../.scratch/splatmap-v2/manual-acceptance-20261024/05-inspect-castle.json),
[result](../.scratch/splatmap-v2/manual-acceptance-20261024/05-inspect-castle.result.json).

**Follow-up:** inspect the four Level Instance world references to identify the
actual authoring level before assessing terrain compatibility.

## 8. Locate Castle authoring levels and assess the workflow boundary — 20:29

**Actions:** read Level Instance references through MCP, return to the test map,
and inspect the existing Castle scene JSON on disk. No maps were saved or edited.

**Live references confirmed:** `Lvl_02_Barracks`, `Lvl_02_Courtyards`,
`Lvl_02_CastleWalls`, and `Lvl_02_ThroneRoom`, all under
`/Game/Levels/Game_Levels/Level_02_Instances/`.

**Historical exported-scene evidence:**
`AGP-Content/Content/ExportedScenes/lvl_forsaken_castle/Lvl_Forsaken_Castle_Level.json`
contains 977 actors, four native Landscape records, and terrain mesh actors
`SM_Landscape_ForsakenCastle_01` through `_04`. Mesh 01 has no exported
components in that file; 02–04 have FBX references and WorldGridMaterial.
These are observations of the existing export, not a fresh export or a complete
live inspection of the child levels. Two Landscape transforms include rotation.

**Implication:** the current preparer supports one axis-aligned native Landscape
and one named runtime mesh. Full Castle export is outside that supported input
shape. A successful single-Landscape acceptance test will not establish that all
four Castle sections can be prepared together. The full team workflow must
resolve section handling and placement explicitly before claiming Castle support.

**Evidence:** [MCP request](../.scratch/splatmap-v2/manual-acceptance-20261024/06-castle-instances.json),
[MCP result](../.scratch/splatmap-v2/manual-acceptance-20261024/06-castle-instances.result.json).

## Intended iterative authoring workflow — clarified, not yet executed

1. Keep the native Unreal Landscape as the editable source for paint and sculpt.
2. For geometry changes, export a fresh FBX from that Landscape and preserve it
   as the authoring export. Record the exact Unreal export settings.
3. Produce and verify a separate corrected runtime FBX. The current acceptance
   plan uses Blender for this handoff. Check transforms, positions, UVs, and
   normals through the actual AGP importer. No corrected FBX has been produced
   from new geometry during this run.
4. Maintain the matching scene mesh reference through Unreal import/reimport,
   as required by the current scene-export/preparation contract; validate the
   material assignment and placement instead of accepting FBX-generated defaults.
5. Export current scene records and painted weights using normal TGA export.
   Prepare both runtime inspection scenes from that fresh export.
6. Run conversion before installing the corrected runtime FBX, since conversion
   can overwrite it with the source copy. Verify hashes, review deployment,
   build, and fully restart AGP for comparison.
7. A paint-only change can reuse verified geometry only when geometry is
   unchanged; it still requires fresh paint export and preparation. Sculpting
   requires a fresh geometry export and correction.

**Next proposed execution:** start with a fresh native-Landscape FBX export in
the existing testing map, rather than treating import of the old FBX as proof of
the authoring workflow. Obtain user permission before performing that export.

## 9. Fresh Landscape geometry export — 20:33:34

**Authorization:** user approved a fresh FBX export and consideration of a minimal
importer fix if feasible. Export into a new evidence folder; preserve previous
authoring and corrected runtime FBXs. No repaint or sculpt is authorized by this
particular step.

**Planned action:** through MCP, use Unreal's `LevelExporterFBX` on the current
world with only its native Landscape selected. Restore the previous selection.
Export binary FBX 2013 with ordinary -Y front convention (`force_front_x_axis`
false), vertex colors enabled, LOD and collision disabled. Record effective
options, source actor transform, engine version, size, and SHA256. No map save.

**Output folder:**
`AGP-Content/.scratch/splatmap-v2/validation/manual-acceptance-20261024/fresh-fbx-01/`.

**Importer feasibility inspection:** the bundled dependency currently contains
public headers and Debug/Release `.lib` files. The AGP conversion helper receives
already imported normal/tangent arrays and normalizes them. Verify whether the
dependency exposes node transform information or implementation source before
proposing a small correction there. Do not infer a safe direction fix from
normalization alone.

**Execution result:** export succeeded, no task errors, 904,624 bytes.
SHA256 `74C11CFB907142AA6D4ACEDE28887277A46A3501CF857BBAEE8C75AFA0023BBC`.
The output folder contains `Lvl_Splatmap_Testing2.fbx` and `export.json` with the
effective settings. This is fresh geometry export of the unchanged starting
Landscape, not the later required sculpt acceptance pass. The starting map,
previous authoring FBX, and corrected runtime FBX retain their recorded hashes.
The previous actor selection was restored and no map was saved.

**Evidence:** [export script](../.scratch/splatmap-v2/manual-acceptance-20261024/export_fresh_landscape.py),
[MCP request](../.scratch/splatmap-v2/manual-acceptance-20261024/07-export-fresh-fbx.json),
[MCP result](../.scratch/splatmap-v2/manual-acceptance-20261024/07-export-fresh-fbx.result.json).
FBX and metadata are retained in the content-repository output folder above.

## 10. Check the fresh FBX through AGP's actual importer

**Action:** run the existing `Bin/Tests/Debug/RuntimeCurrentProbe.exe` with the
prepared runtime scene, the new FBX path from step 9, and `--mesh-normals`.
This mode loads through the bundled importer and checks normal alignment without
starting the game or deploying the fresh FBX.

**Result:** 15 scene actors, two master materials, 16,129 imported vertices, one
mesh element; mean imported normal `(6299.74, 91.5923, -6299.14)`;
area-weighted face/vertex normal alignment `0.217138`; exit code `1`.
The existing threshold is `0.8`. Fresh ordinary Unreal export therefore
reproduces the known downstream normal defect. Export success alone is not a pass.

**Evidence:** [failing probe log](../.scratch/splatmap-v2/manual-acceptance-20261024/08-fresh-import-normals.log).

**Control:** run the same command against the previously corrected runtime FBX;
record its output alongside the failing result to distinguish the raw-export
failure from a general probe/environment problem.

**Control result:** corrected FBX imported 16,129 vertices in one element; mean
normal `(-0.00259603, 0.915923, 0.00863752)`; alignment `0.999565`; exit code `0`.
See [passing control log](../.scratch/splatmap-v2/manual-acceptance-20261024/09-corrected-control-normals.log).

## 11. Assess a minimal importer fix

**Actions:** inspect AGP's dependency headers, mesh conversion seam, and available
importer implementation files in local AGAI/TGE checkouts. No code or libraries
were modified in any repository. This is a feasibility assessment of a previously
diagnosed defect; a full replacement-importer implementation has not begun.

**Findings:**

- AGP has public importer headers and prebuilt Debug/Release libraries, with no
  matching implementation source in its dependency folder.
- The local AGAI/TGE implementations report version 2.0.5 but expose the older
  `LoadMeshA`/`LoadMeshW` interface. AGP's dependency exposes `LoadMesh` returning
  `FbxImportStatus`. Identical version numbers do not establish source compatibility.
- The older AGAI mesh-reading code copies FBX normal arrays directly; this does
  not identify the exact transform operation inside AGP's current binary.
- AGP's `AssetHelper::ConvertVertex` normalizes the vectors it receives after
  importing. Normalization cannot remove translation contamination.
- Per-element transforms are exposed in AGP's headers, so a caller-side
  correction could be investigated. It must first establish the exact baked
  transform, coordinate conversion, and tangent/normal behavior on representative
  meshes. Blindly subtracting a guessed translation or recalculating normals is
  not established as a safe minimal fix.

**Assessment:** a small fix inside matching importer source is plausible, but
the installed dependency cannot currently be patched as a verified one-line
source change. Replacing it with these older sources would expand scope.
Continue the documented FBX correction handoff unless the user chooses a
separate importer investigation. A robust importer fix could eventually remove
the correction requirement, once proven against fresh exports and other assets.

## Next checkpoint

Fresh native-Landscape FBX creation is proven and documented. Direct import of
that fresh export fails the normal check; the historical corrected mesh passes.
No source or runtime FBX was replaced. No paint or geometry edit has been made.

Next proposed step, awaiting permission: inspect the new export in Blender and
produce a separate corrected runtime candidate, then compare its imported
positions, UVs, and normals against this fresh source before any deployment.

## 12. Continue acceptance and inspect installed Blender

**Authorization update:** the user requested continuing the workflow to completion
while documenting every step, checking for Blender and installing it if absent.
Routine steps within the accepted plan may proceed; required human visual
judgments and system approval requirements still apply.

**Discovery:** Blender already exists at
`C:/Program Files/Blender Foundation/Blender 5.1/blender.exe`.
`--version` reports Blender 5.1.2, build `ec6e62d40fa9`. No Blender download or
installation is needed. No Blender MCP tools are exposed in this session.
Use the installed application's Python interface for FBX correction and keep
using the verified MCP connection for Unreal editor operations.

**Next action:** import a copy of the fresh FBX into a clean background Blender
session, preserve its normals, record object/parent transforms and UV structure,
and save the inspection `.blend` and JSON in the run's scratch evidence folder.

**Result:** Blender imported one mesh named `Landscape`, with no parent, 16,384
source vertices, 31,752 triangles, custom normals, and UV layer `TextureUVs`.
Dimensions are `(126,126,23.3203125)` metres. The object's matrix has translation
`(-63,63,0)` metres. Source vertices are distinct at component borders; AGP's
importer merges them to 16,129. Do not equate these two vertex counts without
considering merging.

**Evidence:** [inspection JSON](../.scratch/splatmap-v2/manual-acceptance-20261024/10-blender-inspection.json),
[inspection log](../.scratch/splatmap-v2/manual-acceptance-20261024/10-blender-inspection.log),
`10-source-inspection.blend` alongside those files.

## 13. Prepare a separate Blender correction candidate

Apply object transforms into mesh data, preserve custom normals and UV values,
export a mesh-only FBX with baked axis conversion and centimetre units, then
reimport it in Blender. Verify through the actual AGP importer before accepting
the candidate. All output stays in the run's scratch evidence folder.

**UI availability check:** initialized the installed computer-use runtime and
attempted `sky.list_apps()`. It failed because the native pipe was unavailable.
No Windows input was sent. Unreal MCP remains available. Investigate its exposed
editor APIs for the TGA command; do not replace normal-export acceptance with
handwritten scene serialization.

**Candidate result:** Blender exported `11-blender-corrected.fbx`, SHA256
`2678032DC379623DE5F97B83623AC8A7E4EDD7F4B8EF703F5265C708822680AC`.
Its Blender bake retained UV values and reported zero world-position error.
Custom normals remain after reimport. Reimport includes an axis-conversion
rotation; the runtime import must verify the resulting coordinates.

AGP's existing `--mesh-normals` probe passed with alignment `0.999565` and
16,384 imported vertices. Its existing geometry comparator exited `1` because
the source import merged to 16,129 vertices while the candidate retained 16,384;
it stops before coordinate comparison when the counts differ. This is not a
geometry-equivalence pass. Follow up with indexed triangle-corner comparison,
which is insensitive to vertex duplication while still checking positions/UVs.

**Evidence:** `11-blender-correction.json`, `11-blender-correction.log`, and
`11-reimport-inspection.blend` in the run folder;
[normal check](../.scratch/splatmap-v2/manual-acceptance-20261024/12-blender-candidate-normals.log),
[count-limited comparison](../.scratch/splatmap-v2/manual-acceptance-20261024/13-blender-candidate-geometry.log).

## 14. Inspect exposed editor automation APIs

MCP inspection confirms Landscape render/import heightmap and weightmap APIs are
available. ToolMenus exposes menu inspection/editing but no command-execution
method in the inspected Python surface. The TGA export functions are C++ module
members without UFUNCTION exposure. A normal TGA command has not been invoked.
Keep that gate pending rather than substituting old or handwritten scene data.

**Evidence:** [capability query](../.scratch/splatmap-v2/manual-acceptance-20261024/08-editor-capabilities.json),
[result](../.scratch/splatmap-v2/manual-acceptance-20261024/08-editor-capabilities.result.json).

## 15. Verify indexed triangle geometry and diagnose missing UV1

Built a scratch `GeometryRoundtripProbe` using the actual AGP importer. It compares
95,256 indexed triangle corners, sorting by quantized position/UV keys for stable
pairing and then checking unquantized values. It accepts duplicated vertices
without weakening position/UV tolerances. No production importer code changed.
The initial sandbox build failed on compiler environment access; the approved
unsandboxed v145 build succeeded with missing-PDB warnings from the bundled
library. A source-versus-itself control returned zero errors.

**First candidate result:** positions and UV0 match exactly, but UV1 differs by
up to `1`. The candidate remains rejected. Source FBX inspection revealed a
`LayerElementBumpUV` named `WeightmapUVs`, mapped ByVertice/Direct. Blender's FBX
importer only loads `LayerElementUV`, explaining why the inspection showed one
UV layer and the ordinary roundtrip lost the second channel.

**Follow-up:** restore the preserved source WeightmapUVs as explicit UV1 on the
matching imported control points, then export/reimport and repeat the actual
AGP checks. Keep the first candidate as failure evidence. Added this observed
caution to the manual acceptance guide.

**Evidence:** `14-corner-probe-build.log`, `15-indexed-corner-comparison.log`,
`16-fbx-uv-structure.json` and `.log` in the run folder; diagnostic source/project
under `Intermediate/SplatmapV2Verification/GeometryRoundtripProbe.*`.

## 16. Verify the revised Blender handoff

Restored `WeightmapUVs` from the original FBX's ByVertice/Direct BumpUV data as
an explicit second UV layer on the matching Blender control points. Exported
`17-blender-uvsafe.fbx` and reimported it: both `TextureUVs` and `WeightmapUVs`
are present and custom normals survive.

**Candidate SHA256:**
`8659EFC49AB8A4D193EC9DC85F0E596DA8E29D9581101D1103232ACB280614BE`.
**AGP geometry check:** all 95,256 triangle corners compare with zero position,
UV0, and UV1 differences. **AGP normal check:** alignment `0.999565`, pass.
Different merged vertex counts are retained as an observation; triangle-corner
equivalence establishes the rendered geometry/UV match.

**Evidence:** `17-blender-correction.json`, `17-reimport-inspection.blend`,
[geometry/UV check](../.scratch/splatmap-v2/manual-acceptance-20261024/18-uvsafe-comparison.log),
[normal check](../.scratch/splatmap-v2/manual-acceptance-20261024/19-uvsafe-normals.log).

## 17. Prepare the fresh-paint pass

MCP confirmed the target layers are Snow_Dunes, Snow_Rough, and Snow_Cliff, with
heightmap/weightmap render and import APIs available. Read their signatures;
no paint or sculpt has been applied yet. Plan a small, recorded patch near a cube
landmark, preserving the rest of the weights. Back up source assets first.

Computer Use recovery (`list_windows`) also failed with native-pipe unavailable.
Asked the user asynchronously to restore that connection or handle the normal
TGA export dialog when ready. Continue independent work; no normal TGA export
has been claimed or substituted.

**Backups made:** original map, authoring/runtime FBXs, and previous normal export
copied into content validation `before-authoring-changes/`.

**Fresh source mesh imported:** `/Game/Meshes/Landscape/Test/Lvl_Splatmap_Testing2`,
using the fresh raw Unreal FBX with material/texture import disabled, no lightmap
UV generation, and normals/tangents preserved. Assigned to the existing empty
terrain actor at identity transform and saved the map. Bounds are
`[-6300,-6300,-757.03125]` to `[6300,6300,1575]` cm. The native Landscape material
is assigned to its mesh slot. The mesh actor is temporarily hidden in the editor
to prevent overlapping authoring surfaces. Native Landscape geometry and paint
remain unchanged. See `20-fresh-mesh-import.json` and MCP request/result 11.

**Paint preparation attempt:** creating a transient render target succeeded, but
setting `force_linear_gamma` failed because that property is not exposed to
Python. No Landscape mutation occurred. Retry with the exposed target-gamma
setting. Updated the request runner to treat nested Python `success=false` as
a failed command, even when the outer MCP transport succeeded.

**Baseline weight capture succeeded:** 127×127 Snow_Cliff samples over world XY
`[-6350,-6350]` to `[6350,6350]` cm, chosen so sample centres lie on terrain
vertices. Raw red values range 0–90. Full samples saved in
`21-before-paint-weights.json`; transient render target retained in the editor.

**Controlled paint action starting:** draw a 7×7 full-weight patch into the
captured target at pixels x35–41/y70–76, approximately world X -2800..-2200,
Y 700..1300 cm, north of cube landmark SM_Cube4. Verify that only these 49 target
pixels change before importing Snow_Cliff on edit layer 0. Do not save until a
fresh native weight capture confirms the intended bounded change.

**User response:** they will handle the normal TGA export dialog when ready.

**First transient patch attempt rejected:** the canvas draw cleared background
pixels, changing 9,424 samples, including 9,375 outside the patch. The guard
stopped before calling Landscape import; native paint is unchanged. Reconstruct
the full original target from the captured linear samples, then draw the patch
and require the exact 49-pixel difference before touching Landscape data.

**Retry succeeded:** reconstructed the original render target from captured
linear samples, then drew the patch. Exactly 49 transient samples changed and
zero outside it. Imported Snow_Cliff into edit layer 0 without saving.

**Fresh native verification:** recaptured the Landscape after editor updates.
Exactly 49 samples changed at x35–41/y70–76; zero changed outside the patch.
Patch samples now have weight 255. Saved the map only after that check passed.
See `22-paint-import.json`, `23-native-paint-verification.json`, and MCP requests
13/15. Native geometry was unchanged in this paint-only pass.

**Next:** capture changed overview, then have the user invoke the normal TGA
toolbar export to `AGP-Content/Content/ExportedScenes/`. The command appends the
lowercase level-name folder. Existing export was backed up before authoring.

## 18. Capture the changed view and run content regression suites

MCP `eyes_overview` captured the saved patch at the same camera/size as the
starting overview. The image shows a distinct blue square in the previously
red/pink region. Human visual comparison remains pending.

**Evidence:** [changed overview](../.scratch/splatmap-v2/manual-acceptance-20261024/16-painted-overview-image-0.png),
[starting overview](../.scratch/splatmap-v2/manual-acceptance-20261024/04-baseline-overview-image-0.png).

Ran the current content regression scripts while waiting for the normal export:

- `Tools/Tests/ContentScripts.Tests.ps1`: pass, exit 0. Content copying,
  conflict handling, and batch copy/conversion/error handling.
- `Tools/Tests/PaintedLandscapeRuntime.Tests.ps1`: pass, exit 0. Atlas placement,
  texel centres, scene/material/camera preparation, ordinary material/behavior/
  skylight preservation, source preservation, overwrite/layout/hole guards.

Logs: `24-content-scripts-tests.log` and `25-preparation-tests.log` in the run
folder. These tests do not replace a fresh scene export or the separate
multi-proxy fixture.

The currently installed TGA plugin DLL SHA256 is
`7B3BDEB5B95FC35049CCDF686E3F0021825F5145AF2E106CB593E9C27217A648`.
Record it with the fresh exporter log; older ticket hashes are historical.

**Additional Blender QA:** source and revised reimport have identical physical
XY bounds ±63 m and Z bounds approximately -7.5703125..15.75 m. All corner normals
are unit length within about 2e-7. Zero corner normals oppose their faces; mean
face/corner dot approximately 0.998606. See `26-blender-surface-qa.json` and log.

**Fresh export verification prepared:** `VerifyFreshPaintExport.ps1` checks one
native Landscape, four 64×64 component masks, unique filenames, actual consistent
channel allocation, exactly one terrain mesh reference, hole-free atlas layout,
all shared RGBA samples, and the expected 49 full-weight Snow_Cliff samples.
It has not run against a fresh export yet. User export remains pending.

## 19. Investigate the device's exporter binary

**User report:** the visible exporter still offers editing selected terrain
material export data. The user identified `AGP-Content/ExporterUpdateBranch` as
the authoritative changed exporter.

**Read-only checks:** content checkout is on `ExporterUpdateBranch`, HEAD
`032f100`; no plugin-source changes are pending. Current source menu defines
`TGA → Export Level to JSON`. No matching selected-terrain-material dialog was
found in the current plugin or Unreal tools source. MCP exposes a class named
`TGATerrainLayerTextures`, absent from current plugin source. Installed module
BuildId matches Unreal, but that does not prove correspondence to branch source.

**Decision:** treat source/binary correspondence as unresolved and rebuild the
plugin from the current checkout before fresh-export acceptance. Stage only the
plugin source/resources/content/config and descriptor in the run's scratch
folder; do not copy old binaries/intermediates. Use UE 5.7 RunUAT `BuildPlugin`
with Win64, `-NoXGE`, `-NoHotReloadFromIDE`. Record the build log and deployed
hashes. Asked the user to cancel the terrain-material dialog before restart.

**Build result:** UE 5.7 RunUAT BuildPlugin succeeded, exit 0, about 103 seconds.
Unreal selected its supported MSVC 14.44 toolchain; this is the plugin build,
separate from AGP's required v145 solution build. Build log:
`28-exporter-build.log`; staged source hashes: `28-exporter-source-hashes.json`.
Read-only process-module inspection confirmed the running editor loaded the
project-local `UnrealEditor-TGAUnrealPlugin.dll` path.

**Deployment starting:** back up installed DLL/PDB/module manifest into
`exporter-before-install/`, request graceful editor close, verify process exit,
copy successful package binaries, compare hashes and BuildId, then relaunch the
saved test map. No plugin source change was needed for this rebuild.

**Repository inspection limitation:** a sandboxed `git diff --stat` on the
modified LFS map triggered the Git LFS clean filter, which could not write its
temporary object under AGP-Content `.git/lfs/tmp`. No staging or commit was
requested. Use approved access for later LFS-backed status/diff inspection.

**Deployment completed:** editor exited normally. Installed DLL, PDB, and module
manifest have hashes identical to the successful build package, BuildId
`47537391`. New DLL SHA256:
`1A214F2A3307FFAFCE47B17A856E4BD412B82DBACC52A0BF4ADAD4AE86A2C705`.
See `29-exporter-deployment.json`.

**Post-restart MCP verification — 20:58:30:** saved test map loaded with 15 actors
and the new terrain mesh reference intact. `TGATerrainLayerTextures` is no longer
exposed, confirming the stale binary/class discrepancy was removed. Restored
temporary editor-only hiding of the mesh actor to keep native paint visible.
Asked the user to invoke `TGA → Export Level to JSON` with the normal content
export parent folder. Waiting for the resulting files/dialog confirmation.

## 20. Prepare independent allocation and fixture access

Native-name property inspection through MCP confirmed the weightmap texture and
allocation fields are protected from Python. Existing public C++ Landscape
accessors expose them. Prepared a separate Editor-only scratch plugin,
`AGPAcceptanceTools`, to describe real allocation/texture references and share
weights between two proxies in the explicitly named disposable exporter fixture.
The mutation helper refuses any map whose name lacks `Splatmap_ExporterFixture`.
It does not serialize/export scenes or change the production exporter.

Build in isolated scratch package while the normal paint export is pending.
Do not install/restart or switch maps during the user's pending export action.
Source is under `acceptance-helper/`; build log `30-helper-build.log`.

**Build result:** isolated helper package succeeded, exit 0, about 59 seconds.
No helper installation or restart performed while the user's export is pending.
It will be used for independent native allocation evidence and the separate
shared-weightmap fixture after the fresh-paint runtime pass.

## Current continuation checkpoint

- Working editor: Unreal 5.7.4, correct rebuilt ExporterUpdateBranch DLL loaded;
  existing test map open, saved fresh mesh assignment and verified 49-sample
  Snow_Cliff patch. Terrain mesh temporarily hidden in editor only.
- Fresh raw FBX and corrected Blender candidate with preserved UV0/UV1 are
  verified; runtime mesh has not been replaced or deployed in this run.
- Normal paint-export files still have their old timestamp. Waiting for user to
  run **TGA → Export Level to JSON**, choose the normal ExportedScenes parent,
  and dismiss the completion message. Do not run conversion/preparation from
  the older JSON or switch maps while this human step is pending.
- Once export completes: preserve its JSON/PNGs/log; run
  `VerifyFreshPaintExport.ps1`; establish native allocation evidence with the
  helper; run conversion, install the verified Blender correction after it,
  prepare both scenes, review deployment, build, and restart actual AGP.
- Complete textured/RGB/debug comparisons and obtain human visual judgment.
- Then perform the separate intentional geometry pass, fresh FBX export and
  correction, reimport/reference update, fresh normal TGA export, preparation,
  reviewed deployment, and actual AGP checks. Never reuse the old corrected
  mesh after sculpting.
- Create and export the disposable shared-weightmap/preservation fixture using
  the local helper. Keep it separate from single-Landscape runtime preparation.
- Optional repeat check is not yet selected. Keep tickets open until all required
  gates and final user Unreal comparison pass.

**Evidence checkpoint archived:** copied 86 top-level artifacts, this run record,
and the indexed-corner probe source/project into
`AGP-Content/.scratch/splatmap-v2/validation/manual-acceptance-20261024/evidence/`.
Large build caches and compiler outputs were not included in that evidence copy.

**Human gate currently pending:** the correct exporter is installed and verified,
but the normal source JSON still has its previous 2026-10-24 13:34:47 timestamp.
Fresh export confirmation/files are required before the next dependent step.
This run is incomplete; no runtime deployment or final signoff is claimed.

## 21. Verify the fresh normal TGA paint export

**User confirmation:** exported with the rebuilt plugin. Source JSON timestamp
is 2026-10-24 22:32:56 local, length 87,663 bytes.

**Verification result:** `VerifyFreshPaintExport.ps1` passed: 15 actors, four
Landscape components, exactly one terrain mesh reference, four unique 64×64
mask filenames, consistent allocation R=Snow_Rough/G=Snow_Dunes/B=Snow_Cliff,
1,020 shared RGBA comparisons with zero mismatches, and all 49 expected
full-weight Cliff samples present. Scene SHA256:
`97097B8DB7B7731FB3DA56DD370B355535BADE2D516DCEB61DE44E35C872704B`.
See `27-fresh-export-verification.json` for mask hashes and actual allocations.

**Preservation/conversion preparation:** preserve the complete fresh export and
current Unreal log under content validation `paint-export-01/`. Inventory and
back up existing runtime FBXs that differ from files mirrored by ExportAssets.
Record those paths/hashes before running the established conversion batch.

## 22. Convert assets and resume after approval-service interruption

`ExportAssets.bat` completed successfully, exit 0. Its four fresh weightmaps
were converted to linear RGBA DDS with mipmaps. Fresh JSON/PNGs and the Unreal
log were preserved first in `paint-export-01/`. The only differing runtime FBX
the batch would overwrite was the corrected test terrain; backed it up under
`before-conversion-fbx/`. See `31-conversion-fbx-overwrites.json` and
`32-export-assets.log`.

The next corrected-FBX install/preparation command was not executed because
automatic approval review hit its usage limit. This was a review-service
failure, not a finding that the action was unsafe. No bypass was attempted.

**2026-10-25 00:41 resume:** user requested continuation. Verified conversion
log, current runtime hash `2002A493...5686` (the raw source mirrored by the batch),
candidate hash `8659EFC4...14BE`, and absence of preparation log 33. Retry the
authorized candidate install and fresh preparation, then review outputs.

**Retry result:** corrected runtime mesh installed with exact verified SHA256
`8659EFC49AB8A4D193EC9DC85F0E596DA8E29D9581101D1103232ACB280614BE`.
Fresh Surface preparation succeeded, exit 0. Metadata matches fresh source scene
hash `97097B8D...704B`, current mesh hash, 127×127 atlas, current layer allocation,
world bounds ±6300 cm, and 8 m surface repeat. Both inspection scenes contain
15 actors and two master materials, with the terrain actor at identity.

## 23. Review and deploy the paint pass

The initial direct PowerShell preview was blocked by local script execution
policy. Retried using the documented per-process `powershell.exe -NoProfile
-ExecutionPolicy Bypass -File SyncContent.ps1 ... -WhatIf`, exit 0.

Preview lists 11 destinations: four component mask DDS files, fresh normal scene
JSON, two prepared scene JSONs, preparation metadata, combined mask DDS, corrected
terrain FBX, and `Textures/T_Ground_Snow_NormalMap.dds`. The last is unrelated
conversion output and is excluded. Preserve the ten scoped old engine files,
record current authoring/corrected-FBX provenance, then deploy with explicit
`-OverwriteChangedFiles -ExcludeRelativePath Textures/T_Ground_Snow_NormalMap.dds`.
Check every copied file hash against its source afterward.

Evidence: `33-fresh-paint-preparation.log`, `34-deployment-preview.log`,
`35-paint-deployment.log`, `35-deployed-paint-hashes.json`, and
`before-paint-deployment/` in the run evidence folder.

**Deployment result:** copied exactly ten reviewed files; every deployed hash
matches its source. Unrelated ground normal and destination-only files retained.
Started required `Game.sln` Debug x64/v145 build, log 36.

## 24. Load local allocation evidence helper

Now that the user's export is complete and preserved, restart Unreal on the
saved test map with the scratch helper supplied through supported `-PLUGIN=`
and `-EnablePlugins=` command-line options. This avoids installing its files or
adding an enabled-plugin entry to the team's project. Use it to capture native
layer/channel/texture allocation independently of exporter JSON, then retain it
for the later separate shared-weightmap fixture. No geometry mutation yet.

**Build result:** Game.sln Debug x64/v145 succeeded, seven existing warnings,
zero errors; log `36-paint-game-build.log`. Relinked the existing actual-renderer
probe against current libraries successfully; log `38-runtime-probe-build.log`.

**Helper result:** Unreal loaded the scratch plugin using command-line options,
without project plugin installation/configuration changes. Captured all four
native allocation records and texture references. Every saved native layer,
texture index, and channel exactly matches the fresh exporter JSON. See
`37-native-allocations.json` and MCP request/result 19.

## 25. Check the deployed paint scenes through actual AGP import/rendering

Run the rebuilt RuntimeCurrentProbe against the actual deployed mesh and each
prepared scene, then inspect raw-RGB versus textured rendering invariants.
Capture logs before starting the actual game and human visual comparison.

**Both scene checks passed:** actual deployed FBX imports with coherent normals;
both scenes construct through DirectX with all ten surface/noise bindings,
8 m mapping `[0.00125,0.00125,3.5,3.5]`, and DEBUG 0/2 respectively. Logs 39/40.

**Rendered invariants passed:** switching textured to raw-RGB base color changes
1,865 albedo pixels and zero world-normal, roughness, metalness, or AO pixels.
Actual renderer log `41-paint-rgb-surface.log`; pass images under
`.scratch/splatmap-current/rgb-surface-diagnosis/`.

**Independent native texture readback:** used Unreal's TextureExporterPNG on the
four loaded weightmap texture objects identified by the helper. No TGA serializer
or generated atlas was used for those captures. Compare their decoded RGBA bytes
against all preserved fresh TGA masks, then retain both as evidence. MCP request
20 and `native-mask-readback/` record source object paths and output files.

**Byte comparison passed:** all 16,384 decoded native/exported RGBA pixels match
exactly, zero mismatches across all four component masks. See
`42-native-mask-byte-comparison.json`.

**Human comparison preparation:** positioned Unreal at `(0,0,15120)` cm,
rotation `(-90,90,0)` and captured a matching 1280×720, FOV 90 reference via MCP
(`22-unreal-paint-comparison-image-0.png`). Started actual Game.exe with the fresh
RGB scene selected. Process was responsive with a TM Game window, but the user
could not see it. This launch does not count as an observed visual pass.

**Visibility recovery:** gracefully close the invisible instance and explicitly
relaunch with normal window style through approved interactive-user execution.
Verify process owner/window before requesting the visual judgment again.

The hidden test instance (PID 29872) did not exit after a normal close request.
Automatic review rejected forced termination without explicit scope-specific
approval, citing loss of runtime state. Asked the user; they explicitly approved
force-closing that test PID and relaunching visibly. Retry only that identified
process after verifying its executable path. Saved source/export/evidence data
remain on disk; no termination of other processes is authorized by this approval.

**Visible relaunch result:** old PID 29872 owner was `CodexSandboxOffline`.
After the approved force-close, launched PID 2324 as `TROREEN/tarik`. Approved
desktop-user inspection confirms window title `TM Game`, nonzero window handle,
responsive process, session 1. Default sandbox inspection cannot enumerate that
desktop's window, so use the approved user context for future game launches.
Asked the user to confirm visible RGB patch/landmark correspondence. No visual
pass has been recorded yet.

**Human result:** AGP is visible, but the user reports a mismatch. Asked whether
it concerns paint/cube registration, colors/lighting, or terrain/camera. Hold the
intentional geometry change until this mismatch is understood. Automated import
and binding passes do not override this failed/pending visual gate.

**Read-only height baseline:** first packed-height capture failed its range
sanity check without modifying the map. A later capture after editor frames
returned 16,129 samples, range 31,799–34,784, corresponding exactly to the recorded
terrain Z bounds -757.03125..1575 cm; no zero samples. Saved
`44-before-sculpt-heights.json`. The exact cause of the first failed capture is
not established; retain its failed result as evidence. No sculpt applied.

**Mismatch investigation evidence:** actual Game log confirms the terrain loads
`Lvl_Splatmap_Testing2` and binds `MM_Landscape_Inst`. Deployed FBX hash remains
the verified Blender candidate. Converted the existing actual-renderer 128×128
albedo readback PPMs to PNG without color processing for inspection; these are
probe renderings, not screenshots of the user's game window.

## 26. Investigate visible red/green-region mismatch

**User detail:** the mesh match is uncertain; the square appears closer to the
green region in AGP than the red region shown in Unreal. User explicitly asked
for a screenshot of the actual game.

**Capture:** native computer-use screenshot pipe remained unavailable. Used a
read-only Windows PrintWindow capture of the verified desktop-user game window.
No mouse/keyboard input or UI Automation was performed. Captured 2560×1440,
4,083 sampled unique colors, with PrintWindow success. See
`46-visible-game-window.png` and `.json`. This is the actual live game window.

**Evidence correction:** initial PPM-to-PNG conversion assumed 128×128, but the
probe's actual client/backbuffer was 120×89. Corrected the conversion to parse
dimensions and validate byte count. Raw PPMs were unchanged. The earlier
converted PNGs are superseded and must not be treated as shape evidence.

**Ranked checks:**
1. Unreal's named-layer debug ordering may differ from AGP's packed RGB ordering;
   predict a red/green meaning difference while square/landmark placement stays.
2. World mask coordinates may be transposed; predict square displacement relative
   to fixed cube landmarks, not merely different band colors.
3. Camera or imported geometry orientation may differ; predict cube/shape mismatch
   after matching the top-down poses.

Inspect the Unreal material graph first. Do not alter generated masks to conceal
an unexplained discrepancy. The visual acceptance gate remains open.

**Graph evidence:** native layers are ordered Snow_Dunes, Snow_Rough, Snow_Cliff,
all LB_AlphaBlend. Traced the DEBUG-selected branches through reroutes and
MakeMaterialAttributes to constants: Dunes=(1,0,0), Rough=(0,1,0), Cliff=(0,0,1).
See `47-unreal-material-graph.json`, `48-unreal-debug-layer-order.json`,
`49-debug-blend-connections.json`, `50-debug-color-connections.json`, and
`51-named-debug-colors.json`.

**Confirmed comparison mistake:** Unreal DEBUG=1 displays named ordered layer
contributions; AGP DEBUG=2 displays raw packed R=Rough/G=Dunes/B=Cliff. Their
red/green meanings differ. Do not treat that color difference alone as proof of
shifted paint. Placement still needs a matching-view comparison.

**Matching diagnostic view:** make a separate engine-local inspection scene from
the prepared RGB scene, changing only MM_Landscape_Inst DEBUG from 2 to 1.
Preserve canonical textured/raw-RGB scenes and all mask bytes. Select that copy
and normally restart the desktop-user game for a matching named-contribution
comparison and live-window screenshot. This is an explicit diagnostic view,
not an edit to reinterpret generated masks.

**Setup correction:** the first diagnostic-copy command searched for the instance
in MasterMaterials, but DEBUG is stored on master `MM_Landscape`. That command
failed before writing the copy/settings. An incorrectly sequenced restart
therefore reopened raw RGB. Corrected the lookup, created the separate copy,
verified DEBUG=1 and the selected path, and only then requested the new restart.
Canonical generated scenes and mask data were untouched.

## 27. Verify terrain shape independently of lighting

**User clarification:** corner markers only establish placement/orientation;
the main concern is whether the imported terrain retained its hills and valleys,
which differing reflections make difficult to judge. Defer adding markers until
the direct shape check is available.

**Action:** prepare an isolated C++ probe using AGP's actual bundled FBX importer.
For every imported vertex, compare its height to the captured native 127×127
height field at corresponding world XY, with 0.002 cm tolerance. Also report
errors under all eight grid rotations/reflections rather than guessing an axis
flip. This directly compares native source shape against actual imported mesh,
unlike earlier FBX-to-FBX correction-equivalence checks.

Inputs: deployed corrected runtime FBX and `54-native-heights.txt`, derived
without modifying values from `44-before-sculpt-heights.json`. Diagnostic code
is under `Intermediate/SplatmapV2Verification/NativeLandscapeRegistrationProbe.*`.
No production importer code, geometry, or masks changed for this test.

**Result:** all 16,384 imported vertices lie exactly on the expected XY grid.
The assumed UE XY=AGP ZX orientation has mean height error 480.022 cm and max
2178.12 cm. The rotate90 alternative has exactly zero mean/RMS/max height error
for every imported vertex; all six other alternatives have substantial error.
Thus shape is preserved under a 90-degree orientation discrepancy, rather than
lost hills/valleys. See `55-native-to-agp-shape.log`. This falsifies the claim
that earlier FBX-to-FBX equivalence alone established native registration.

**Next test:** export another preserved raw FBX from the unchanged native source
with Unreal's Force Front X Axis enabled, then run the same native-height check.
The previous export used the default -Y-front option. Existing terrain tool
documentation already distinguishes this axis convention. Keep both raw exports
and do not rotate masks or deploy an unverified correction.

**Force Front X raw result:** export succeeded to `fresh-fbx-frontx-01/`, SHA256
`314AE1207C8CBA3D7F7841C50B0DE6C216EBD01C414D5160F17D5E635EA4697A`.
Actual importer versus native height field now matches in the intended identity
mapping with zero XY/height error at all 16,129 imported vertices. All rotated/
reflected alternatives fail. See `56-frontx-native-shape.log`.

**Next:** Blender correction must retain this front-axis convention as well as
normals and both UV channels. Import the new Force Front X source, bake transforms
and units, preserve WeightmapUVs, and export X-forward/Z-up with baked space
transform. Verify against native heights, raw-front-X positions/UVs, and normal
alignment before deploying it. Previous candidate remains failure evidence.

**Revised Blender candidate verified:** `58-blender-frontx-corrected.fbx`, SHA256
`93AB13014248D8891C09575766CED6E2E704123492FFB2D97FE9294ECDADA0C6`.
Blender export X-forward/Z-up preserves the intended source convention. Actual
native-height comparison passes: zero height error, max horizontal rounding
0.000976563 cm. Raw-front-X versus corrected FBX has the same horizontal rounding
and exactly zero UV0/UV1 difference across all indexed corners. Normals align at
0.999565. Logs 57–61 and Blender records retain the exact handoff.

**Correction deployment:** preserve the rotated candidate, install only the
native-verified corrected mesh, prepare from the unchanged fresh paint export,
record the new raw/corrected hashes and axis convention, and review deployment.
Native authoring geometry/paint and mask bytes remain unchanged during this fix.
Updated the manual guide with the observed axis-convention requirement.

**Deployment result:** preview contained only corrected FBX and metadata; copied
exactly those two files, with unrelated texture excluded and destination-only
diagnostic scenes retained. Deployed mesh hash is `93AB1301...A0C6`. Logs 62–64.

**Actual camera basis check:** scratch CameraPoseProbe reports post-import
world rows right `(0,0,1)`, up `(-1,0,0)`, forward `(0,-1,0)`, position
`(0,15120,0)` in AGP. This differs by 180 degrees around the view direction from
the assumed basis used for the earlier Unreal reference. Match Unreal to the
actual runtime basis using pitch -90/yaw -90, rather than assuming yaw +90.
Record this synthetic-camera roundtrip discrepancy separately; no production
camera/importer code was modified. See `66-actual-camera-pose.log`.

**Next capture:** restart actual game to release the old mesh cache; capture
new runtime and native views using the verified matching camera basis. Terrain
shape is numerically verified against native heights; visual signoff remains
pending. No corner markers or sculpt edits have been added yet.

**Independent next-pass preparation:** capture the current native packed 16-bit
height field read-only as a baseline for the later intentional sculpt. Do not
change geometry while the paint visual checkpoint is pending.

## 28. Lighting-free evidence for the user's terrain-shape question

**Question:** does the terrain retain its hills and valleys, rather than merely
having the correct placement? The different reflections make visual judgment
difficult. Answer this with native heights and actual imported vertices.

**Live capture review:** `67-game-frontx-contributions.png` and
`31-unreal-actual-camera-reference-image-0.png` have corresponding terrain
outlines, cube landmarks, and painted patch after the front-axis correction.
Highlights and blended colors still differ; this is not full visual signoff.

**Read-only probe extension:** added optional CSV output to the scratch
`NativeLandscapeRegistrationProbe.cpp`. It writes native versus actual imported
height at every vertex using UE world XY, retaining the existing assertions and
all eight orientation checks. No production code, source landscape, paint,
materials, or deployed assets changed during this step.

**Execution details:** the first MSBuild attempt failed with file-tracker access
denied inside the sandbox (`68-shape-plot-probe-build.log`). Rebuilding outside
the sandbox succeeded (`69-shape-plot-probe-build.log`). Ran the probe against
the deployed `Content/Meshes/Landscape/Test/Lvl_Splatmap_Testing2.fbx` and the
preserved native height field `54-native-heights.txt`. Results are
`70-native-agp-height-comparison.log` and `70-native-agp-height-samples.csv`.

**Result:** all 16,384 imported vertices have zero native height error. Maximum
horizontal rounding is 0.000976563 cm. All 16,129 unique native grid positions
are present; duplicated component-border vertices agree. All seven alternative
rotations/reflections fail, so this result also verifies registration.
Earlier indexed-corner comparison (`61-frontx-geometry-uv.log`) confirmed that
the correction preserves all 95,256 indexed corners and both UV channels.
Normal alignment remains 0.999565 (`60-frontx-blender-normals.log`).

**Lighting-free plot:** `PlotTerrainHeights.ps1` renders the CSV as two height
maps with the same color scale and orientation, blue low and orange high.
The output `71-terrain-height-comparison.png` was inspected: corresponding hills
and valleys match. It is a plot of independently read source/imported data, not
a screenshot or a shaded engine rendering. The script checks complete grid
coverage and rejects inconsistent duplicated vertices. Script execution policy
initially blocked execution; a process-local policy override was used, without
changing machine policy. A PowerShell array-enumeration error was corrected
before the successful plot. These attempts did not modify source assets.

**Material investigation remains open:** installed UE source
`MaterialExpressionLandscapeLayerBlend.cpp` confirms alpha-blend layers lerp in
order from a zero initial value. AGP currently assumes a continuous base layer.
Whether that distinction contributes to this scene's color difference still
requires checking the actual base weights. It does not invalidate the measured
geometry result. Do not silently change either blend contract.

**Status:** terrain shape and registration pass for the corrected deployed mesh.
Material/lighting visual parity, intentional sculpt iteration, the separate
preservation fixture, and final human acceptance remain pending.

## 29. Resume acceptance and isolate the base-color difference (2026-10-25)

Read the next-chat handoff, acceptance plan, workflow and version-control guides,
shared rules, and existing run results before resuming. No completed setup was
repeated. Unreal PID 21844 is alive; Game is no longer running. Direct Unreal MCP
is now exposed and reports ready, map Lvl_Splatmap_Testing2, 15 actors. Read-only
editor inspection confirms the scratch helper is loaded and camera is still
(0,0,15120), pitch -90/yaw -90/roll 0. Evidence: 72-resume-editor-state.json.

Engine branch remains SplatmapV2; Content remains ExporterUpdateBranch, with
pre-existing modifications retained. Content Git inspection first hit ownership
and LFS sandbox-write errors; command-local safe.directory and disabled LFS
filters allowed read-only status without changing global config or repository
files. Both remotes remain Troreen/AGP and Troreen/AGP-Content. No staging,
commits, publication or Perforce operations were performed.

Used the repo-local diagnosing-bugs discipline. Existing matched captures show
the color symptom; the next narrowed check compares only named base-color
contributions using independently captured native mask bytes. Prior ranked
hypotheses remain: debug meanings (already corrected), blend semantics, then
lighting/normal/exposure differences. A blend mismatch predicts different Dunes
red contributions even before lighting, while fully weighted Cliff agrees.

Ran `powershell.exe -NoProfile -ExecutionPolicy Bypass -File
.scratch/splatmap-v2/manual-acceptance-20261024/CompareBaseBlend.ps1`.
It compares UE alpha-over-zero Dunes*(1-Rough)*(1-Cliff) with AGP continuous-base
(1-Rough)*(1-Cliff) at all native component texels, and exits nonzero when they
differ by more than one byte. Result is red: 16,335/16,384 samples differ,
maximum linear red delta 1, mean 0.416545. All 49 fully painted Cliff samples
agree. Dunes is not uniformly 1: component ranges are 164..216, 0..215,
0..216, and 9..215 in bytes. See 73-native-base-blend-comparison.json.
This proves a material formula mismatch, not the magnitude of a screen-space
lighting error; filtering, tone mapping and shading are excluded from this check.

Installed UE 5.7 source MaterialExpressionLandscapeLayerBlend.cpp confirms the
zero initial value and ordered alpha lerps; existing material graph evidence
48..51 establishes this material's ordering/colors. No production shader or
native material was modified. Asked the user whether to retain the specified
continuous base with this documented exception or require painted-base parity.
Continue independent runtime and surface evidence while awaiting that decision;
full visual signoff remains open, and sculpt is still deferred at this checkpoint.

## 30. Final axis-corrected runtime probes and matched surface captures

Ran the existing RuntimeCurrentProbe on the deployed corrected FBX with the
canonical Surface scene, then the RGB scene with `--rgb-surface`. Both exit 0.
Logs: 74-final-frontx-surface-runtime.log and
75-final-frontx-rgb-surface-runtime.log. Both construct the actual DirectX scene,
bind all ten textures and verify 8 m mapping [0.00125,0.00125,3.5,3.5], DEBUG 0/2.
RGB changes 1,851 albedo pixels; world normals, AO, roughness and metalness each
change zero pixels. This supersedes the pre-axis-correction 1,865-pixel result.
Deployed FBX still hashes to 93AB13014248D8891C09575766CED6E2E704123492FFB2D97FE9294ECDADA0C6.

Preserved current settings as 76-ApplicationSettings.before-surface.json.
Selected the canonical Runtime_Level.json and launched visible Game PID 24020
as the desktop user. Read-only PrintWindow succeeded at 2560x1440, screenshot
78-game-frontx-surface.png and associated JSON. Production logo overlay remains.

Temporarily selected DEBUG=0 on the native material. The Python setter returned
False, but subsequent scalar-array and getter readbacks both confirmed 0.0.
Captured only after verification. The old standalone bridge failed with
`Remote party failed to attempt the command socket connection`; see request
32-unreal-surface-reference.result.json and log 77. Direct MCP eyes_mirror then
succeeded immediately. Saved original image bytes and camera metadata as
79-unreal-frontx-surface.png/json. Both images are 2560x1440, FOV 90 at the
established matching camera. No image content was edited. Restored native DEBUG
to 1.0 and verified readback; no explicit asset save was requested. UE autosave
logged a temporary material copy under Saved/Autosaves during this inspection.

Visual inspection: corresponding outline, landmarks and large terrain features;
Unreal has much stronger highlights and darker valleys, AGP flatter/whiter.
Full surface parity does not pass. These are different render pipelines and the
current lighting is not controlled: read-only native inspection reports white
directional light intensity 10, source angle 0.5357, rotation pitch -46.621078,
yaw -166.256145, roll -169.796657. Preparation intentionally uses intensity 2.
The measured base-blend mismatch does not establish the entire shading cause.

User replied "wait what" to the blend-contract question. Clarified the uncovered
Dunes behavior in plain language and withdrew the premature decision request;
no behavior change was authorized or made. Presented the two actual surface
captures and asked whether to investigate shading first or proceed to sculpt
while retaining an explicit visual failure. Continue evidence inspection while
awaiting that steering. Game is currently left open on the textured scene;
native DEBUG is restored to the original contribution view. Tickets stay open.

## 31. Preserve verified handoff guidance and refresh durable evidence

Read-only surface graph inspection confirms all nine expected Snow texture
objects are connected to three MF_LandscapeLayer calls. Saved graph snapshot
80-native-surface-graph.json and partial function snapshot
81-native-layer-function.json. The latter records node classes, UV scaling
constant 8 and input names, but the function connection API returned empty
arrays: do not interpret those arrays as disconnected graph inputs. The optional
open-editor MCP tool failed because its AssetEditorSubsystem method is absent;
it made no asset changes. No new normal/ORM parity claim follows from this probe.

Backed up the Content preparation guide to 82-PreparePaintedLandscapeRuntime.before.md,
then inserted reviewed guidance 83-verified-axis-uv-handoff.md before Verification.
An exact pre-write SHA256 guard protected pre-existing changes. New Content guide
SHA256 is 0D7488C0989078370116E99313266BFC33DDAD9E7E5D06825241307AE4D6169E.
The update documents Force Front X, Blender X-forward/Z-up, explicit legacy UV1
restoration, actual native-height/UV evidence, and remaining blend/light mismatch.
The correction script remains fixture-specific; it is archived as evidence and
has not been promoted into a general production converter.

Refresh the durable Content evidence in a new resume-20261025 snapshot rather
than overwriting the older archive. Include current run log/handoff and the
scratch probe source/projects. Verify every copied file hash against source.
No sculpt or fixture mutation was made while the visual checkpoint awaits
steering. Their required gates and final human signoff remain pending.

## 32. User authorizes proceeding with visual failure retained; sculpt pass

User selected **Proceed; retain visual failure**. Continue sculpt/export gates;
do not change blend behavior or mark surface parity passed. Durable snapshot
resume-20261025 completed before sculpt: 218 files, 36,505,962 bytes, all hashes
match (84-resume-archive-result.json). Later sculpt evidence will need archiving.

Live packed height recapture exactly matches all 16,129 preserved baseline
samples; range 31799..34784. Evidence 85. Backed up current source umap and
terrain uasset into before-sculpt/ before mutation.

Applied one localized parabolic mound at grid (93,38), world XY (3000,-2500) cm,
radius 500 cm, peak +200 cm. Reconstructed every packed RG target pixel because
Canvas begin-draw clears the target. Before import, all 16,129 target samples
exactly match the intended field, precisely 69 changed and zero outside radius.
Native import succeeded inside an editor transaction; see apply_sculpt_patch.py
and 86-sculpt-import.json. No save until independent readback.

Independent native export after editor updates exactly matches expected heights:
69 changed samples, zero outside, zero expected mismatches, peak +200 cm.
Evidence 87-after-sculpt-native-heights.json. The native probe input
87-native-heights-after-sculpt.txt stores packed uint16 samples in the same
format as baseline 54 (initial cm/header formatting was corrected before use).
Paint preservation readback and map save follow before fresh Front-X FBX export.

**Paint and save:** transient paint baseline state was absent in this editor;
the first attempted check stopped at that lookup without saving. Instead exported
all four native weightmap texture objects independently into
after-sculpt-mask-readback/. Every PNG is byte-identical to pre-sculpt native
readback (88-sculpt-paint-preservation.json), proving every channel unchanged.
Saved the verified map successfully.

**Fresh geometry:** exported native Landscape with ForceFrontXAxis=true to
Content validation sculpt-fbx-frontx-01/Lvl_Splatmap_Testing2.fbx. SHA256
051E1BBE1BD405C9CAB79E060BF0CF33134ABA2CBFAEF324EEF2A864E43FE86F.
Export report's map_saved=false means that export script did not save the map;
the separate preceding verified save succeeded. Adapted the existing inspection
and correction recipes to new filenames, retaining the source raw FBX unchanged.
Blender 5.1.2 candidate 91-blender-sculpt-corrected.fbx SHA256
29BBC7E277C3ACBA9517763EA39FC7B2D9CFCBA8DFD6522F3D50E455C9DCA72E.

**Actual importer verification:** 92-sculpt-native-agp-shape.log reports zero
native height error at all 16,384 imported vertices, max XY rounding 0.000976563 cm.
Normal alignment is 0.999512 (93). Indexed-corner comparison preserves 95,256
corners, max position error 0.000976562 cm, both UV differences zero (94).
The first corner invocation omitted its scene argument and failed JSON parsing;
retained 94-sculpt-geometry-uv-wrong-arguments.log and corrected the invocation.
The raw export has the previously characterized bad importer normal transform;
its alignment printed by the corner probe is not the corrected candidate's.

**Unreal mesh update:** replaced only the backed-up terrain asset using new
corrected FBX, material/texture import disabled. Verified native bounds within
0.002 cm before saving; source import provenance now explicitly points to
91-blender-sculpt-corrected.fbx. InterchangeAssetImportData uses
script_extract_filenames (the initially tried scripted_extract_filenames method
does not exist). Saved asset successfully, preserved temporary editor hiding.
Evidence 95-sculpt-mesh-reimport.json and import_sculpt_terrain.py.

**Visual evidence:** 96-unreal-sculpt-contributions.png/json records the native
view after sculpt; new mound near world XY (3000,-2500) appears separately from
the unchanged Cliff square. Generated lighting-free native/imported sculpt plot
97-sculpt-terrain-height-comparison.png from CSV 92 using the existing plotter.

**Normal exporter checkpoint:** asked the user to run TGA > Export Level to JSON
to Content/ExportedScenes (exporter appends lvl_splatmap_testing2). MCP lacks a
callable normal TGA command, so do not substitute a synthetic JSON or stale export.
Await completion before conversion/preparation/deployment. Keep the current map
open for that dialog; the separate exporter fixture has not started.

## 33. Fresh normal sculpt export, preparation and deployed runtime

User replied exported. Canonical JSON timestamp is 2026-10-25 01:33:05 local,
SHA256 53222834068DF11F266C15BD30899FF9E3066CAD7257B024069897881A72FCCC.
Normal export validation passes 15 actors, four components, one terrain mesh,
unique masks, correct Rough R/Dunes G/Cliff B, 1,020 matching shared RGBA
comparisons, and all 49 Cliff patch samples. Report 99. The existing verifier
initially wrote its fixed report filename 27; immediately renamed the new report
99 and restored original 27 exactly from the already verified pre-sculpt archive.
Preserved fresh normal source output in Content validation sculpt-export-01/scene.

ExportAssets.bat succeeded, exit 0 (100). Its final pause briefly left the process
session active; attempted stdin was closed, subsequent poll confirmed normal exit.
Installed new verified candidate 91 only after conversion, retaining the overwritten
runtime source as 101. Surface preparation succeeded (102); both source/mesh hash
guards pass, 8 m repeat and directional intensity 2 retained. Recorded current raw
authoring hash and Blender/native/UV provenance in regenerated preparation.json.

Deployment preview 103 listed five scoped files plus unrelated ground normal.
Backed up all five engine destinations, excluded the ground normal, copied exactly
five files and verified every hash (104). Game had already exited when normal
close was attempted; no process was terminated. Game.sln Debug x64 v145 succeeded
with zero warnings/errors (105).

Deployed actual DirectX probes pass for Surface and RGB (106/107): expected
mapping, all ten texture bindings; RGB changes 1,851 albedo pixels and zero
normals/AO/roughness/metalness pixels. Started fresh visible Game PID 1380 on
Surface and captured 108-game-sculpt-surface.png. Normally closed it and restarted
on canonical raw RGB as PID 8700, captured 109-game-sculpt-rgb.png. Both capture
calls report success, 2560x1440. Source masks remain unchanged, while the sculpt
now appears in the runtime. Numeric shape/UV checks pass; full shading parity
remains failing by the user's instruction. No implementation changes were made.

The separate exporter-preservation fixture is next. It must not be prepared or
deployed as multi-proxy runtime content. Existing sculpt map/assets remain saved.

## 34. Disposable fixture setup and editor recovery

Saved fixture copy with EditorLoadingAndSavingUtils.save_map to
/Game/Levels/Test_Levels/SP/Lvl_Splatmap_ExporterFixture_20261025. The API wrote
the new map but left the original world active; report 110 records that fact.
Original saved sculpt umap SHA256 remained
3812491A8B3CE6387EADA3B6F883C74AA5A543B0A43CD7AE7609545E817F141B.
Loading the fixture then crashed Unreal at EditorServer.cpp:1938, World Memory
Leaks. Preserved full log as 111-fixture-editor-crash.log before restart.

The log identifies the exact retained reference: FPyReferenceCollector holds
AssetExportTask_8, whose Object is native Weightmap_15, retaining Landscape and
the old World/package. This was an avoidable retained Python export-task reference
from inspection, not evidence of source corruption. Original saved umap hash
still matches. Future snippets should scope UObject references inside functions,
clear task.object after exporting, and release old references before changing maps.
No force-kill was used; the editor exited after its own fatal assertion.

Restarted Unreal visibly directly on the saved fixture with the same external
AGPAcceptanceTools plugin flags, new PID 32872. No source project/plugin install
or configuration changes. Confirm ready/map before further fixture mutation.

## 35. User reports apparently flipped UVs/colors

User observed that colors did not match and suspected flipped UVs. At that
moment actual Game was canonical raw packed RGB (DEBUG=2), while native evidence
96 shows named layer contributions (DEBUG=1). Explained their different red/green
semantics without dismissing the registration concern.

Regenerated only the engine-local contribution inspection scene from the fresh
sculpt RGB scene, changing master DEBUG 2 to 1. Backed up previous inspection
JSON as 112. Normally closed Game PID 8700 and restarted matching contributions
as PID 34580. Capture 113-game-sculpt-contributions.png/json. Canonical scenes,
mask bytes and production shader behavior are unchanged.

Inspected 113 against native 96: Cliff square occupies the same location relative
to cubes; broad paint boundaries correspond. This supports matching mask
registration rather than a spatial flip. AGP's excess yellow/pink remains and
is consistent with its continuous red Dunes base versus UE's painted Dunes
alpha, already measured by report 73. Terrain mask addressing is world-position
based (AGP ZX mapped to UE XY), not mesh UV0/UV1. Preserved UV0/UV1 comparisons
alone do not prove paint placement, so the matching-view check is kept separately.
No claim of full color or texture-normal parity; user authorized retaining failure.

Restarted Unreal process 32872 is alive but has no main window handle and its log
has stalled after engine initialization, before remote execution readiness.
Asked whether a startup/recovery dialog is visible and requested its actual
message; do not dismiss recovery prompts blindly. Fixture remains a saved copy
with one Landscape: duplication/shared-weightmap setup and normal export are
still pending. Original sculpt map and runtime evidence remain safe.

## 36. Fixture ready for normal exporter command

Unreal startup completed and direct MCP is ready on the fixture, 15 initial
actors. User later answered no startup dialog was visible; no dialog action was
taken. Setup ran inside a function to avoid persistent UObject references.

Duplicated Landscape_0 as Landscape_1 offset +15000 cm X. The guarded helper
assigned identical texture references and layer allocations for all four
corresponding components. Both proxies have same-named component objects,
creating the intended filename-collision test. Added Fixture_Camera and an
existing BP_ScriptedEvent_Test actor labeled Fixture_Behavior. The behavior
actor includes ScriptedEventComponent_C, LockControlActionComponent_C,
UnlockControlActionComponent_C, WaitActionComponent_C and PlayAudioActionComponent_C,
plus scene/box components. Saved fixture successfully; 18 actors total.
Native shared-reference baseline: 114-fixture-native-baseline.json.

Requested the user's normal TGA export to Content/ExportedScenes, expected
subfolder lvl_splatmap_exporterfixture_20261025. Do not convert/prepare/deploy
this fixture as runtime content. Verify eight unique masks, matching native
bytes and allocations, ordinary actors/materials, camera, and behavior payloads
after normal export. Empty custom payload still needs explicit observation.

## 37. Normal multi-proxy export verification

User replied done. Normal TGA exporter log reports 18 actors exported at
2026-10-24 23:39:58 UTC to
Content/ExportedScenes/lvl_splatmap_exporterfixture_20261025/.
Ran VerifyExporterFixture.ps1 against actual normal output and native snapshot
114. Report 116-exporter-fixture-verification.json passes:

- Two proxies, four components each, eight unique proxy/component filenames.
- All 32,768 decoded RGBA pixels match independent native texture readback;
  shared proxy files have correct identical pixels without overwriting one another.
- Every exported layer/channel/texture index matches native allocation.
- All 14 original non-Landscape actor JSON records match the fresh sculpt
  source exactly, including their component/material records.
- Original master material records match exactly.
- Added camera has expected world position (1000,2000,9000), FOV 90 and TypeID 11.
- All five custom behavior components and their non-empty Custom payloads exist.

The available behavior actor does not exercise an empty Custom object: count 0.
Record that limitation explicitly; do not infer an empty-payload pass. No fixture
preparation or deployment was performed. A further read-only native camera/
behavior inspection request was issued around the export dialog and is pending;
it has not yet produced 115. Native camera position was explicitly set in setup,
but exported behavior values still need independent readback if available.

**Readback completion:** user closed the normal export success modal. Pending
MCP inspection then completed, confirming native camera position and FOV 90
(115). Blueprint serialization event methods are not exposed to Python. Direct
get_editor_property attempts for serialized keys and common authored-name forms
are unavailable (118/119); no values were changed. Behavior-record presence and
non-empty payload preservation pass, but independent value-by-value behavior
parity is not established. Empty payload remains unexercised.

**Return:** function-local fixture inspections released UObject references;
Python collection preceded a successful return to the original sculpt map.
Saved original umap hash remains 3812491A...F141B. Restored camera and temporary
editor hiding of the terrain mesh, avoiding z-fighting after map reload. Kept
native DEBUG=1 and actual Game on the matching engine-local contributions scene.
Final editor state is recorded in 120-final-editor-state.json. Fixture was never
prepared or deployed.

**Ticket/documentation status:** updated authoritative Content tickets 02 and 03
with current measured results, retained both in-progress and final visual gate
open. Source backups and update text are evidence 117. Updated manual acceptance
signoff table with measured passes and explicit failed/unexercised checks.
No production blend change, new implementation, staging, commit, push or
Perforce submission. Optional 16 m repeat check was not run; saved repeat is 8 m.

## 38. Resumed-run checkpoint and durable archive

Final snapshot created at Content validation/manual-acceptance-20261024/
resume-final-20261025: initial 301 files, 58,663,829 bytes, every copied SHA256
verified. Report 121-final-archive-result.json. It includes fresh sculpt raw and
corrected FBX, fixture umap/export, all current evidence, backups, probe sources/
projects and current docs. Earlier snapshots remain intact. This section and
archive-result receipt are copied afterward with manifest entries refreshed.

Current measured passes: fresh paint and normal export, localized sculpt and
unchanged paint, Front-X Blender correction and preserved UV0/UV1, exact native
height registration through actual AGP importer, fresh preparation/provenance,
reviewed scoped deployment, v145 build, both actual runtime modes and visible
restarts, and primary multi-proxy/shared-mask preservation checks.

Open: full visual parity FAIL (user chose proceed while retaining it), final
human signoff, empty-payload fixture coverage not exercised, independent behavior
value readback unavailable. Optional repeat comparison skipped. Matching
contribution views support matching paint placement; do not equate differing
colors with a demonstrated UV flip or claim all shading causes are resolved.
Tickets 02/03 remain in-progress. Runtime repeat is 8 m. No production behavior
changed during this resumed run. Original local work preserved; no staging or
publication performed.

Archive finalization: adding the receipt exposed a Windows PowerShell
ConvertFrom-Json array-wrapper quirk; the first refresh nested the original
manifest under value/Count. An initial flatten attempt rejected that wrapper
without changing files. Corrected the wrapper, restored a flat manifest, and
verified all 302 archived file hashes, including the receipt and refreshed log.
No evidence file was lost or altered by the manifest-format correction.

## 39. Ground floater requirement: confirmed current failure

User clarified that ground floaters must be affected by the ground mask. This
matches the intended behavior in Docs/SplatmapMVPPlan.md: terrain and floaters
share the world-to-mask transform, surface phase, layers and projected-normal
convention. At the same horizontal world position they must sample the same
painted distribution, independent of mesh UVs and height. Earlier acceptance
used the twelve cubes only as registration landmarks and missed this gate.

Read-only native MCP inspection confirms all twelve SM_Cube actors currently
use /Game/Meshes/Landscape/Test/MI_Landscape_SplatMapTesting, whose parent is
/InterchangeAssets/Materials/FBXLegacyPhongSurfaceMaterial. Despite the same
asset name, this is a different material from the native Landscape's
/Game/LevelPrototyping/Materials/MI_Landscape_SplatMapTesting, parent
/Game/Materials/MM_Landscape. Evidence 123-native-ground-floater-materials.json.

Checked all three deployed scenes with CheckGroundFloaters.ps1. Report
122-ground-floater-material-audit.json: 12 floaters in each scene, ZERO sharing
the terrain ground-material parent. Surface, raw RGB and contribution scenes
all fail. Each floater retains FBXLegacyPhongSurfaceMaterial; terrain uses
MM_Landscape_Inst parent MM_Landscape. Existing actual runtime log 106 confirms
the cube material assignments. The preparer changes only its named terrain mesh
material (PreparePaintedLandscapeRuntime.ps1:216), preserving cube assignments.
The runtime SceneWorldBuilder resolves those material parents independently.

MM_Landscape already samples mask, surface maps and noise from world position
(AGP ZX = UE XY), so its mapping is suitable for shared floaters. Current
floaters are not bound to that shader/mask. This is a material assignment gap,
not proof of a UV flip. No material, scene, source asset or production code was
changed by this inspection. Required correction is to bind explicitly identified
ground floaters to the same prepared ground material/mask and mapping, retain
unrelated ordinary materials, then verify a boundary-overlapping and rotated
floater in actual runtime views. That fix and visual test have not been made.

Audit execution wrote its JSON and returned failure as intended. A direct script
invocation was blocked by local execution policy; use the documented process-local
Bypass invocation. An auxiliary summary command initially had a PowerShell empty
pipe parse error and was corrected; the saved audit results were unaffected.
