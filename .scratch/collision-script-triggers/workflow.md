# Collision script triggers workflow

## State

- Request: implement code-authored Enter-to-Play trigger, registry configuration, focused tests, and an opt-in runtime demo.
- Current stage: implementation complete; automated verification complete; visual runtime interaction remains unverified.
- Authoritative scope: `spec.md`; it supersedes the original broader plan for separate EventActor configuration, world filtering, trigger replay policy, and importer/exporter changes.
- Tickets: `issues/01-code-authored-activation.md`, `issues/02-exported-trigger-configuration.md`, and `issues/03-runtime-verification.md`.
- Original base: `4fc24b56fa6d3589a73b4d17b02ad0d767407122` on branch `collision-system`; no commits after the base.
- Initial working tree contained only this feature's untracked planning directory. Existing workflow records are being updated in place.
- Work remains unstaged and uncommitted.

## Implementation notes

- The trigger, its collider, sequence coordinator, and action components share one Actor.
- The trigger stores only EnteringActor, a world-local collider ID, and its listener token; it resolves sibling/entrant/sequence components at use time.
- The sequence's Play result controls collider disablement; the trigger does not add replay state.
- Runtime demo is enabled only when `AGP_ENABLE_COLLISION_SCRIPT_TRIGGER_DEMO` is set to a nonempty value; it adds a Player collider only in demo mode.
- Follow-up request: Debug Game scenes now draw every active scene collider, independently of the demo switch. Disabled collider outlines remain gray. Successful trigger activation logs sequence and entrant Actor names.

## Verification status

- `MSBuild.exe Game.sln /m:1 /nologo /v:quiet /p:Configuration=Debug /p:Platform=x64 /p:MultiProcessorCompilation=false /p:CL_MPCount=1` succeeded after the final `getenv_s` opt-in check change. It emitted one existing unused-parameter warning in `Game.cpp`.
- `ScriptedEventsTests.vcxproj` Debug x64 build succeeded. The first run exposed a missing `ServiceLocator.Initialize()` in the custom component fixture; the fixture now initializes services and clears the World before service teardown. The corrected controlled run exited 0 and printed `PASS: scripted sequence ordering, movement, persistence, claims, replay and destruction`. Captured output is `test.stdout.txt` and `test.stderr.txt` in this feature directory.
- Follow-up verification: after scene-wide Debug collider outlines and success-only trigger logging were added, serial Debug x64 `Game.sln` build succeeded (exit 0), `ScriptedEventsTests.vcxproj` build succeeded, and `ScriptedEventsTests.exe` exited 0 with the same PASS line. Its stderr includes `[GameFramework] Collision scripted event 'EncounterVolume' started when Actor 'Player' entered.` and corresponding successful activation logs for `ExportedEvent`, `RetryVolume`, and `FilteredVolume`. Captured follow-up output is `test-followup.stdout.txt` and `test-followup.stderr.txt`.
- `RunPublicHeaderIsolation.ps1 -Configuration Debug -CollisionOnly` passed for the modified public ColliderComponent header and collision headers. The check log is `Intermediate/CollisionHeaderIsolation/Debug/build.log`.
- Visual GUI verification remains pending: confirm all scene collider outlines are visible in a Debug Game run, that the consumed trigger remains gray, and that the success message is visible in the GameFramework log.
- `CollisionTests.vcxproj` Debug x64 build and executable succeeded (`PASS: CollisionTests`). `CustomComponentTests.vcxproj` Debug x64 build and executable succeeded (`PASS: custom property import and SpinComponent creation`).
- Bounded startup smoke: launched `Bin/Debug/Game.exe` with `AGP_ENABLE_COLLISION_SCRIPT_TRIGGER_DEMO=1` against the checked-in current scene, observed the process alive after 8 seconds, sent `WM_CLOSE`, and observed exit code `-1`. This checks opted-in startup only; no visual interaction was performed, so box entry, sequence effect, and F4 reload still need a human GUI check.
- `git diff --check` passed, and a trailing-whitespace scan passed for new/untracked files. Final status: 9 modified tracked files and new docs, component source/header, and feature records/logs; generated library outputs were restored. All edits remain unstaged and uncommitted. `git log 4fc24b56fa6d3589a73b4d17b02ad0d767407122..HEAD --oneline` is empty.
- Independent spec and standards reviews found no remaining code, spec, or standards issues after scope correction.

## Final cleanup requested by user

- Supersedes the earlier demo requirement: remove the collision-script demo switch,
  fixture Actors/actions, and demo-only Player collider from Game.cpp.
- Keep the reusable trigger, registry/build membership, automated tests, activation
  logs, and scene-wide Debug collider outlines.
- Replace demo instructions with the short Unreal authoring handoff in
  Docs/ScriptedEventCollisionTriggers.md. Collider shape mapping, actual custom
  component export round trip, and animation target import remain future work.
- Planning records and captured test output stay local and are excluded from the
  requested source commit. Pre-existing collision-shape demo is outside this cleanup.
- Final cleanup verification: Debug x64 Game.sln build passed after removing the
  scripted demo. ScriptedEventsTests, CollisionTests, and CustomComponentTests
  executables each passed. Staged diff check passed. No new visual claims.
