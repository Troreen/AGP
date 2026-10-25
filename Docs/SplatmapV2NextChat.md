# Splatmap V2 manual acceptance: next-chat handoff

Handoff recorded 2026-10-25. Resume the existing acceptance run; do not start a
new implementation or repeat completed setup. Read AGENTS.md and its required
guides first, then [the run log](SplatmapV2AcceptanceRun-2026-10-24.md), especially
sections 26–28, and [the acceptance plan](SplatmapV2ManualAcceptance.md).

## Current checkpoint after resumed run (sections 29–37)

**Latest user clarification and failure (section 39):** ground floaters must be
affected by the ground mask. All twelve SM_Cube actors currently FAIL this in
all three deployed scenes: they use MI_Landscape_SplatMapTesting with parent
FBXLegacyPhongSurfaceMaterial, not MM_Landscape. Unreal contains two different
same-named material assets: cubes use the imported one under Meshes/Landscape/Test;
native terrain uses the one under LevelPrototyping/Materials, parent MM_Landscape.
The preparer currently reassigns only its named terrain mesh. Check script and
reports 122/123 confirm the gap; no fix was applied. Earlier acceptance treated
these cubes only as landmarks and missed the floater gate. Correct explicitly
identified floaters to share the prepared ground material/mask/world transform,
preserving unrelated materials, then test boundary alignment and rotated normals.
This is separate from UV flips and the previously retained shading failure.

- Intentional sculpt/export/Blender/preparation/deployment/restart is complete.
  New runtime FBX hash is 29BBC7E277C3ACBA9517763EA39FC7B2D9CFCBA8DFD6522F3D50E455C9DCA72E;
  source raw Front-X is 051E1BBE1BD405C9CAB79E060BF0CF33134ABA2CBFAEF324EEF2A864E43FE86F;
  fresh normal source JSON is 53222834068DF11F266C15BD30899FF9E3066CAD7257B024069897881A72FCCC.
  Logs 99–109 cover fresh export validation, conversion, preparation, five-file
  scoped deployment, zero-warning/error build and actual runtime/render probes.
- Sculpt native-height/UV checks pass; Game screenshots 108 surface and 109 raw
  RGB are saved. User suspected flipped UVs/colors. Switched Game to matching
  named contributions and captured 113: square and broad boundaries align with
  native 96; colors remain different, consistent with measured base blend issue.
  Do not treat raw RGB vs native contributions as equal color meanings.
- Separate fixture normal export is complete. User ran normal TGA command and
  closed success modal. Fixture has two proxies sharing four native textures,
  18 actors, eight unique masks. Report 116 passes 32,768 native RGBA pixels,
  allocations, all 14 original ordinary actors/materials, added camera and five
  non-empty behavior payload records. Empty payload was not exercised. Blueprint
  value-by-value readback is unavailable (118/119); do not claim it passed.
- Fixture map is /Game/Levels/Test_Levels/SP/Lvl_Splatmap_ExporterFixture_20261025,
  export Content/ExportedScenes/lvl_splatmap_exporterfixture_20261025. Never
  prepare/deploy it as multi-proxy runtime support. It was not deployed.
- One editor crash during map switch was caused by Python retaining AssetExportTask
  with native texture Object (exact reference chain log 111). Original saved
  source hash stayed intact. Restarted directly on fixture; later function-local
  UObject inspection and Python collection allowed safe return to original map.
- Live Unreal PID 32872 is back on saved sculpt test map, 15 actors, DEBUG=1,
  terrain static mesh temporarily hidden, matched camera restored. Game PID 34580
  is matching contributions, engine-local fresh inspection JSON. Re-query PIDs.
- Authoritative Content tickets 02/03 updated and remain in-progress. Signoff
  table updated: geometry/runtime/export passes, visual parity FAIL/open,
  empty payload unexercised and final human signoff pending. User explicitly
  authorized proceeding with visual failure retained; no blend change authorized.
- Latest durable archive is intended as resume-final-20261025 under Content
  validation/manual-acceptance-20261024; verify result 121 for completion.
- No more user export dialogs are pending. Remaining work concerns visual/color/
  shading resolution or an accepted specification change, optional empty-payload
  coverage/independent behavior readback, and final human acceptance. 16 m repeat
  comparison skipped as optional; 8 m retained. No commit/publication authorized.

## Historical checkpoints from this resume

**Latest checkpoint: section 32 supersedes the no-sculpt state below.** User
selected **Proceed; retain visual failure**. Sculpt is now applied and saved:
grid (93,38), world XY (3000,-2500) cm, radius 5 m, peak +2 m; exactly 69 heights
changed, zero outside. All four native mask PNGs are byte-identical pre/post.
Backups are under evidence/before-sculpt/. Native heights 87 and report 88.
Fresh preserved raw Front-X source is Content validation
`sculpt-fbx-frontx-01/Lvl_Splatmap_Testing2.fbx`, SHA256
051E1BBE1BD405C9CAB79E060BF0CF33134ABA2CBFAEF324EEF2A864E43FE86F.
New corrected candidate is evidence `91-blender-sculpt-corrected.fbx`, SHA256
29BBC7E277C3ACBA9517763EA39FC7B2D9CFCBA8DFD6522F3D50E455C9DCA72E.
Actual importer checks 92..94 pass: all 16,384 vertices zero native height error,
max XY rounding 0.000976563 cm, 95,256 corners preserved, both UV errors zero,
normal alignment 0.999512. Plot 97 was visually inspected and matches.
Unreal terrain static-mesh asset was replaced from this corrected candidate,
bounds verified and saved, provenance readback explicitly confirms candidate 91.
Native Landscape map saved; DEBUG remains 1 and static mesh remains temporarily
editor-hidden. New native contribution capture is 96. Runtime deployment is
STILL THE PRE-SCULPT mesh (93AB...A0C6); do not claim sculpt deployed.

**Required next action:** user has been asked to run normal
TGA > Export Level to JSON and choose
`C:/Users/tarik/Documents/GitHub/AGP-Content/Content/ExportedScenes` (exporter appends
lvl_splatmap_testing2). Await their "exported" reply; current canonical JSON was
still the older 2026-10-24 22:32 export at the last check. Then preserve/validate
fresh export, convert, restore NEW candidate 91 after conversion, prepare with
new hashes, review scoped deployment, restart Game, rerun runtime and visual
checks. Separate multi-proxy fixture remains unstarted; do not switch map before
the pending export. Visual failure remains explicitly accepted as open work.

- Direct Unreal MCP tools now work; prefer them. The standalone bridge failed
  its command socket connection while direct tools succeeded. Saved failed result.
- Final corrected-mesh runtime probes now pass: logs 74/75. RGB changes 1,851
  albedo pixels, zero normal/AO/roughness/metalness pixels. Mesh hash unchanged.
- Actual matched textured captures: `78-game-frontx-surface.png` (Game) and
  `79-unreal-frontx-surface.png` (native). Shading differs substantially; do not
  claim full parity. UE DEBUG was temporarily 0, verified, captured, then restored
  to 1. No explicit material save; editor logged an autosave in Saved/Autosaves.
- Native masks establish a real base-blend difference: UE painted Dunes alpha
  vs AGP continuous base differs at 16,335/16,384 component texels; all 49 fully
  painted Cliff texels agree. Script CompareBaseBlend.ps1 and report 73 retain it.
- Native directional intensity is 10; prepared runtime intentionally uses 2.
  Base blend, lighting and renderer differences must be separated before a fix.
- User said "wait what" to an early blend-contract question. Explained plainly
  and withdrew it as premature. Then presented matched surface captures and asked
  whether to investigate shading first or proceed to sculpt with visual failure
  retained. Await this steering; no blend changes or sculpt have been made.
- Live Game is now PID 24020 on canonical textured Runtime_Level.json. Settings
  immediately before this selection are backed up as 76-ApplicationSettings.before-surface.json.
  Unreal PID 21844 remains on original map, camera unchanged, native DEBUG=1.
  Re-query PIDs before acting. Earlier Live state below is historical.
- Content team preparation guide updated with measured axes and legacy UV1
  handoff; backup 82 and inserted text 83 preserve exact scope. No production
  converter created. Partial surface graph evidence 80/81 has no parity verdict.
- Durable archive refresh targets a separate `resume-20261025` snapshot under
  the same Content validation root. Check section 31 and copy manifest for result.
- Sculpt, multi-proxy fixture, surface parity, and final human acceptance remain
  open. No staging/commit/push/Perforce operation was performed.

## User intent and authorization

- Finish the manual acceptance plan using Unreal MCP, documenting every step
  starting from when MCP first worked. Narrate meaningful actions and findings.
- The team needs an iterative workflow: edit native Landscape in Unreal,
  create/correct FBX, export scene/masks, prepare and deploy, inspect in AGP.
- Work directly in the existing test map; user explicitly waived the extra
  test-map copy. A separate disposable exporter preservation fixture is still
  required by the plan.
- User authorized MCP installation, local Python remote execution, configuration
  and editor restart, and later said to continue until entirely done. Involve
  the user for visual judgments and normal TGA export dialogs when needed.
- No Git commit, staging, push or Perforce submission is authorized. Preserve
  pre-existing modifications in both repositories.
- Latest question concerned terrain shape, beyond placement: lighting obscured
  hills and valleys. We have now answered this with actual numeric evidence and
  a lighting-free height plot. Do not assume full visual parity has passed.
- User requested this handoff and a new chat; no application or asset changes
  were made merely to hand off.

## Live state at handoff

- AGP workspace: `C:/Users/tarik/Documents/GitHub/AGP`, branch SplatmapV2.
- Content repository: sibling AGP-Content, branch ExporterUpdateBranch (earlier
  inspected HEAD 032f100; recheck before version-control operations).
- Process check at handoff: Game PID 26212 and UnrealEditor PID 21844 alive.
  Re-query before acting; PIDs are not permanent identifiers.
- Unreal map: `/Game/Levels/Test_Levels/SP/Lvl_Splatmap_Testing2`.
  Native Landscape, four 63-quad components, 12 cubes, directional light and
  one terrain StaticMeshActor. Native Landscape at (-6300,-6300,0), scale 100.
- Native material MI_Landscape_SplatMapTesting, parent MM_Landscape. It is in
  the named contribution debug view (DEBUG=1). No sculpt change has been made.
- Native viewport matches actual game top-down basis: location (0,0,15120),
  pitch -90/yaw -90, FOV 90. Earlier yaw +90 reference was wrong.
- Existing terrain static mesh was filled with the fresh source earlier and
  temporarily editor-hidden to avoid z-fighting. Do not mistake that for an
  excluded export. Its source import still refers to the earlier default-front
  raw FBX; update provenance/reimport deliberately for the sculpt pass.
- `Bin/Settings/ApplicationSettings.json` currently selects the separate
  engine-local inspection scene:
  `lvl_splatmap_testing2_inspection/Lvl_Splatmap_Testing2_Contributions_Level.json`.
  This changes master MM_Landscape DEBUG to 1. Canonical generated views remain
  DEBUG 0 (surface) and DEBUG 2 (raw packed masks). Settings backup is in evidence.
- Game has a large hardcoded TGA logo overlay; preserve unrelated production UI.

## Tools and execution constraints

- Unreal 5.7.4 installed under `C:/Program Files/Epic Games/UE_5.7`.
- Unreal MCP installed at `C:/Users/tarik/Documents/UnrealMCP`, commit
  602727c0b5732d6c6fa556a6e6c9913a6c5d5a96; venv Python 3.14, 74 tools.
  Codex MCP entry `unreal` runs that venv Python with `-m unreal_mcp`.
  Local-only remote execution binds 127.0.0.1, TTL 0, enabled with user approval.
- Direct Unreal MCP tools were not exposed last chat. Actual MCP SDK bridge:
  run `C:/Users/tarik/Documents/UnrealMCP/.venv/Scripts/python.exe`
  with `AGP/.scratch/splatmap-v2/manual-acceptance-20261024/mcp_request.py`
  and a request JSON containing `{ "tool": "execute_python", "arguments": ... }`.
  Existing request files show exact schema. It saves result JSON and image files.
  Execute editor scripts using runpy.run_path with absolute workspace paths.
- Native computer-use pipe was unavailable. Do not claim it can click or capture.
  User has handled the normal TGA export dialog. MCP can inspect/set editor state.
- Pure read-only visible Game screenshot fallback:
  `CaptureVisibleGame.ps1 -GameProcessId <current PID> -OutputStem <new name>`
  in the evidence folder. Uses PrintWindow; requires desktop-user execution.
- Sandbox writes only AGP. Sibling content writes, normal desktop-user launch,
  Unreal connection and Visual Studio file tracker have needed tool escalation.
  Use normal approval review, never bypass a rejection. If denied, report it.
- Game launches from sandbox can be invisible. Launch visibly as desktop user
  with Start-Process, working directory Bin/Debug and WindowStyle Normal.
  Close normally and wait; do not blanket force-close. Earlier force approval
  was only for hidden PID 29872, already terminated.
- Blender was already installed: `C:/Program Files/Blender Foundation/Blender 5.1/blender.exe`,
  version 5.1.2. No Blender MCP tool exposed; used background CLI Python.
- External acceptance helper loaded in current editor via command-line flags:
  `-PLUGIN="<evidence>/acceptance-helper-build/AGPAcceptanceTools.uplugin"`
  `-EnablePlugins=AGPAcceptanceTools`. Keep those if restarting for the fixture.
  Source/package are in evidence, not installed into the project.

## Established findings and passes

Evidence root throughout this file:
`AGP/.scratch/splatmap-v2/manual-acceptance-20261024/`.
Durable content validation root:
`AGP-Content/.scratch/splatmap-v2/validation/manual-acceptance-20261024/`.

1. Correct exporter DLL rebuilt from current Content source and installed with
   backups. Old editor DLL exposed stale terrain-edit dialogs. New DLL SHA256
   1A214F2A3307FFAFCE47B17A856E4BD412B82DBACC52A0BF4ADAD4AE86A2C705.
2. Intentional paint pass complete: 49 native samples x35..41/y70..76 changed
   to full Snow_Cliff, outside changes zero. Fresh normal TGA scene export saved
   and validated; canonical source JSON SHA256
   97097B8DB7B7731FB3DA56DD370B355535BADE2D516DCEB61DE44E35C872704B.
   Four distinct 64x64 masks, native channels Rough=R/Dunes=G/Cliff=B, shared
   borders agree. Native texture byte readback matches exported RGBA exactly.
3. Conversion, Surface prep and scoped SyncContent completed. ExportAssets.bat
   overwrites runtime FBX, so restore the verified correction AFTER conversion.
   Excluded unrelated regenerated T_Ground_Snow_NormalMap.dds from deployment.
4. Game Debug x64 v145 build and actual scene probes passed before the final
   axis correction; source regression suites passed. Re-run relevant runtime
   probes for the final corrected mesh before final acceptance.
5. Bundled importer treats normals with translated node transform incorrectly.
   Actual matching importer source is absent; no production importer fix made.
   Baked Blender transforms fix face/vertex normal alignment to 0.999565.
6. Ordinary Blender import loses Unreal LayerElementBumpUV WeightmapUVs.
   Correction script restores it explicitly as UV1 from raw FBX vertex indices.
7. Critical axis finding: earlier default Unreal -Y-front export preserved shape
   but rotated it 90 degrees against native world coordinates. Correct procedure
   is Unreal ForceFrontXAxis=true, Blender X-forward/Z-up with baked space
   transform, transforms/units baked and both UVs preserved.
8. Current verified raw Front-X FBX: Content validation `fresh-fbx-frontx-01/`,
   SHA256 314AE1207C8CBA3D7F7841C50B0DE6C216EBD01C414D5160F17D5E635EA4697A.
   Corrected Blender candidate `58-blender-frontx-corrected.fbx`, and deployed
   `AGP/Content/Meshes/Landscape/Test/Lvl_Splatmap_Testing2.fbx`, SHA256
   93AB13014248D8891C09575766CED6E2E704123492FFB2D97FE9294ECDADA0C6.
9. Direct native heights vs actual AGP importer: all 16,384 imported vertices
   have ZERO height error; maximum XY rounding 0.000976563 cm. Every one of
   16,129 unique native grid points present. Seven rotated/reflected alternatives
   fail. Raw-vs-corrected indexed corner check: 95,256 corners preserved,
   maximum position difference 0.000976562 cm, both UV differences zero.
10. Lighting-free comparison generated from separately read native/imported CSV:
    `71-terrain-height-comparison.png`. Visually inspected, matching shape.
    Read `70-native-agp-height-comparison.log` and samples CSV for measurements.
    Scratch probe source/project under Intermediate/SplatmapV2Verification:
    NativeLandscapeRegistrationProbe, GeometryRoundtripProbe, RuntimeCurrentProbe,
    CameraPoseProbe. Probe optional third argument exports CSV; no production
    code was changed for this height plot.
11. Actual game capture `67-game-frontx-contributions.png` matches outline,
    landmarks and paint placement against `31-unreal-actual-camera-reference-image-0.png`.
    Highlights/blended colors still differ. Earlier references and raw RGB view
    comparisons are superseded, retained as failure evidence.

## Open investigation: shading and debug semantics

- Unreal DEBUG=1 uses named contributions: Dunes red, Rough green, Cliff blue,
  all LB_ALPHA_BLEND in that order. AGP DEBUG=2 is raw packed R/G/B with Rough
  red and Dunes green. Comparing those was a prior mistake, already documented.
- AGP separate diagnostic DEBUG=1 now matches named contribution meanings.
- AGP blend assumes continuous base; installed UE source
  Engine/Source/Runtime/Landscape/Private/Materials/MaterialExpressionLandscapeLayerBlend.cpp
  initializes zero then lerps all alpha layers. Check actual Dunes weights to
  determine whether this explains remaining colors. Do not silently alter the
  specified continuous-base contract or source material.
- Native mask readback PNGs are under `native-mask-readback/`; allocations in
  `37-native-allocations.json`; native material graph traces 47–51.
- Normal/highlight and exposure differences also need controlled comparison.
  Shape pass alone does not prove shading parity. Can use temporary unsaved
  native DEBUG=0 and restored settings for matching surface screenshots.
- Synthetic camera roundtrip has 180-degree roll discrepancy from preparer's
  assumed basis: actual AGP right(0,0,1), up(-1,0,0), forward(0,-1,0), pos(0,15120,0).
  Matching Unreal viewport is established; no camera/math production fix made.

## Remaining work, in order

1. Resolve/characterize remaining shading/color differences and get the user's
   visual judgment using correctly matched debug and textured views. Re-run
   final actual runtime probes after axis correction. No corner markers needed
   for shape proof; they were deferred following user's clarification.
2. Intentional sculpt iteration has NOT started. Native packed baseline is
   `44-before-sculpt-heights.json`, text 54; script capture_geometry_baseline.py.
   Transient builtins state may still exist but verify before using. Proposed
   small change away from paint, e.g. x93/y38 radius5, +200cm peak. Reconstruct
   packed RG target fully (Canvas draw clears target), assert intended changed
   samples and zero outside BEFORE import/save. Cold captures have needed retry.
3. Fresh native ForceFrontX export after sculpt, new Blender correction preserving
   UV1/axes/normals, direct native-height validation. Reimport correct source into
   Unreal terrain static mesh. User handles normal TGA export dialog when reached.
   Convert, restore corrected runtime FBX, prepare fresh export with provenance,
   preview scoped sync, back up, deploy, actual runtime + visual validation.
4. Separate disposable multi-proxy exporter preservation fixture still required.
   Acceptance helper exposes unreal.AGPAcceptanceLibrary.describe_landscape and
   share_fixture_weightmaps(source,target), guarded to fixture map names containing
   Splatmap_ExporterFixture. Duplicate fixture Landscape, share texture references,
   record native allocations, normal export; prove unique proxy filenames, shared
   correct pixels and non-landscape/camera/material/behavior preservation (empty
   behavior payload if available). Do not prepare fixture as supported multi-proxy
   runtime; preparer supports one Landscape only.
5. Update Content team preparation guide with verified axis and legacy UV1 handoff,
   and consider making the proven hardcoded scratch Blender recipe reusable.
   Manual acceptance guide already has axis/UV cautions.
6. Refresh durable evidence archive (earlier archive held 86 older artifacts;
   latest 31–71 evidence and updated scratch probes need archiving). Include log.
   Scope sibling writes through normal approval review.
7. Keep Content authoritative tickets 02-painted-export and 03-runtime-pipeline
   open until required gates and final human signoff pass. Document remaining
   limitations and final status honestly. No automatic completion claims.

## Small gotchas

- Fresh prep rewrites metadata; manually added authoring/runtime FBX provenance
  must be reapplied or incorporated deliberately after each preparation.
- Do not use raw vertex count alone: component seam merging changes counts.
- Runtime RGB/surface probe PPMs are 120x89 client pixels, not assumed 128x128;
  parse P6 header. Earlier striped conversion was corrected and documented.
- Windows python alias cannot run in sandbox; use known UnrealMCP venv Python.
- Plot script initially needed a process-local PowerShell ExecutionPolicy Bypass;
  machine policy unchanged. It is standard data plotting, not screenshot editing.
- Current full plan includes more than the latest user's shape question. Resume
  from these open items rather than treating the shape answer as final acceptance.
