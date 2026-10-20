# Demo removal verification

## Results

Debug x64 Game.sln build passed with Visual Studio 2026 v145. These test projects
built and their executables exited 0:

- CollisionTests
- ScriptedEventsTests
- CustomComponentTests
- GameApplicationTests

The first lifecycle-suite build exposed a stale CollisionDemoComponent compile
item; removing it and rerunning that suite passed. No demo references remain in
Source, Tests, Docs or README. Git diff whitespace checking passed.

## Scope and limits

The demo-free local Game build passed. Additional checkpoint verification used a
temporary Game.cpp with Perforce's existing active-camera audio listener enabled;
local source bytes were restored afterward. Audio-listener behavior was not changed
by this cleanup. No visual, Unreal export, or Release validation is claimed.

Sandbox verification initially encountered linker LNK1101 (MSPDB140.DLL mismatch).
The exact-payload build and test verification succeeded outside the sandbox.
Expected test stderr was evaluated using native process exit codes.

## Independent review

Spec and Standards reviewers independently reviewed the cleanup and plan against
Git base 519d779527b162c31131e292dd9ad007ecc747c2. Both reported no blocking findings,
including reassessment of the final test-project fix. These reviews do not claim
a fresh audit of the whole previously committed collision implementation.

## Commands

Build Game.sln and each named Tests/GameFramework/<suite>.vcxproj with MSBuild:
/p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo /nr:false

Run each corresponding Bin/Tests/Debug/<suite>.exe. Historical core implementation
records remain in ../collision-system and ../collision-script-triggers.

## Import-and-tags increment

Review base: 077d2c947e6fda96d65c8f9c26710da22e92e9e0. This increment leaves
Player creation disabled, preserves the user's new component-scene export and
does not modify the exporter or Unreal assets.

Final results with v145:

- Debug Game.sln: passed.
- Release Game.sln: passed; the historical linking failure did not recur.
- Debug CollisionTests: build/run passed, including the new import tests.
- Debug ScriptedEventsTests: build/run passed.
- Debug CustomComponentTests: build/run passed after isolating its original
  exported property probes from the editable component test scene.
- Debug GameApplicationTests: build/run passed.
- SceneData.h, UnrealSceneImporter.h and GameCollisionLayers.h each compile in
  an otherwise-empty translation unit in Debug and Release, using only Source,
  Source/Engine, Source/Utilities and shared-math include roots. Include logs
  contain no DirectX, GraphicsEngine, Windows or RHI headers.

TDD evidence: real box import failed on the Bounds object; sphere failed as a
placeholder; named filters failed before the map API; invalid dimensions and
unsupported layouts failed before validation. Each became green before the next
slice. The final import suite also covers unequal extents/yaw, mutual masks,
explicit empty masks, unknown/duplicate/malformed tags, capsule preservation,
and successful import of the user's entire current scene. The actual box fixture
is a focused subset of the supplied export. Sphere/yaw/invalid cases are clearly
marked synthetic inputs, not claimed as exports from Unreal.

Independent Spec and Standards reviews have zero remaining findings. The added
yawed rectangular-box test resolved their import-axis/yaw coverage concern.

Commands follow the MSBuild flags above, with Game.sln additionally built with
/p:Configuration=Release. Final verification logs are local under
Intermediate/CollisionUnreal. Existing compiler warnings were not broadened into
this feature. No visual alignment, Player entry, F4 interaction or actual sphere
export round trip is claimed; these remain following-increment checks.

## Player and trigger integration

Player mesh, animator, controller and movement are restored, with an explicitly
approximate radius60 sphere at localY90, Player layer and Trigger mask. Current
settings select the component test scene. AssetRegistry's temporary asset-name
print and navmesh debugbreak were removed. The unrelated audio-listener code and
pre-existing ColliderComponent formatting change were preserved.

Debug Game.sln build passes. After these changes CollisionTests,
ScriptedEventsTests, CustomComponentTests and GameApplicationTests each rebuilt
and ran with exit0. Logs: Intermediate/CollisionUnreal/*-player-build.log and
*-player-run.log. Independent Spec and Standards reviews report zero findings.

Human checks pass: user confirmed "F2 sequence completes", performed the normal
TGA export, and confirmed automatic first-entry activation, gray consumed outline,
and F4 reset restoring green and allowing activation again. Re-entry without reset
does not restart the sequence; this is the intended one-shot trigger behavior.
Inspected the actual updated JSON for Trigger/Player shape tags and exported
CollisionScriptTriggerComponent_C with EnteringActor=Player. Existing actions and
Replayable=true remain intact. No manual scene reconstruction was used.

The new AuthoredCollisionTriggerScene.json is a focused frozen subset of that
real export, retaining the box, coordinator, trigger and one unchanged Wait action.
TestAuthoredCollisionTrigger exercises import -> registry -> builder -> World
updates, rejected Enemy/distant Player, Player entry, collider consumption, action
completion and no re-entry restart. Debug ScriptedEventsTests rebuilt and ran with
exit0 after this addition. This fixture does not prove full visual/audio playback.
Existing tests separately cover initial overlap and retry after rejected Play.

Independent Spec and Standards reassessments of the final test and fixture both
report zero findings. Both confirmed that all five retained component records
and Actor metadata match the real updated export.

Precise Player body fit and real sphere export/visual alignment remain unverified.
Desktop automation is prohibited by the user's instruction. No feature changes
were staged or committed, and nothing was submitted to Perforce.

## Sequence-owned TriggerCollider refactor

Debug Game.sln passes. Debug ScriptedEventsTests, CustomComponentTests and
GameApplicationTests each build/run with exit0 after removal of the bridge type.
Logs: Intermediate/CollisionUnreal/*-trigger-refactor-build.log and *-run.log.
Tests cover optional empty/named references, missing/wrong-type diagnostics,
initial overlap, layer filtering independent of Actor names, successful collider
consumption, rejected-start retry on new entry and scene teardown/replacement.
Both independent refactor reviews report zero findings.

User performed the real export after MCP updated the reusable sequence Blueprint
and map, removed the old instance and deleted its unreferenced Blueprint asset.
Actual JSON has TriggerCollider=TriggerBox on ScriptedEventSequence, retains
Replayable=true and Trigger/Player tags, and contains no CollisionScriptTrigger.
The focused fixture was refreshed from actual records, retaining one Wait action;
ScriptedEventsTests rerun against it exits0 (real-refactor-export-run.log).
Human entry/gray/F4 verification for this refactor is pending. Earlier human
results establish the prior arrangement only. No automated visual checks claimed.
