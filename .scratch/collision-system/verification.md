# Collision system verification

All commands ran from the repository root with Visual Studio 2026 MSBuild and v145, C++20, x64. No staging or commits.

## Passed

- Debug solution: `MSBuild.exe Game.sln /m /nologo /v:minimal /p:Configuration=Debug /p:Platform=x64`. Final default-restored build is recorded in evidence/final-solution-debug.log.
- Historical only: EventBus enhancement tests previously passed Debug/Release. The user subsequently requested the original EventBus unchanged; those tests are archived and are no longer acceptance criteria. Fresh collision integration checks for that revision are recorded below.
- CollisionTests Debug: full shape/filter/lifecycle suite; additional test-project /W4 /WX build/run passed in evidence/03-final-debug-wx-build.log and evidence/03-final-debug-wx-run.log.
- Current CustomComponentTests, ScriptedEventsTests and GameApplicationTests Debug build/run passed. Integrated test projects were built with `/p:BuildProjectReferences=false` after fresh framework/solution builds. Results: evidence/integrated-<project>-debug-build.log and evidence/integrated-<project>-debug-run.log.
- `Tests/GameFramework/RunPublicHeaderIsolation.ps1 -Configuration Debug -CollisionOnly` and the Release equivalent: all three new collision headers independently compile with no backend imports. See evidence/03-header-isolation.log and evidence/integrated-collision-header-release.log.
- Independent Spec and Standards source reviews: zero findings. Both reviewers reassessed the Windows max-macro correction and narrow regression compatibility updates. See review.md and review-scope.md.

## Remaining mandatory gates

- Release Game and CollisionTests executable links fail C1007 (`unrecognized flag '-permissive-' in 'p2'`) and LNK1257 during LTCG. All relevant code compiled. Fresh v145 CommonUtilities Release rebuild did not fix the links. The /GL restart notices name different first triggering members and do not conclusively identify a culprit. This remains an unresolved toolchain/prebuilt-library compatibility gate, not a confirmed collision-source defect. Toolset and vendor files were not replaced. See evidence/integrated-collision-release-build.log and evidence/integrated-solution-release.log, plus the bounded retry logs if present.
- Whole-repository header isolation already failed before collision additions, including missing CommonUtilities include roots and existing renderer dependencies. The baseline is retained in evidence/header-isolation-baseline.log. New-only coverage passes; a full pass is not claimed.
- Legacy GameFrameworkTests cannot compile against current ServiceLocator APIs and absent TGAFBXImporter source paths. Current lifecycle regression suites above were used instead. This older suite was not broadly rewritten.
- Runtime visual/reload verification is unobserved. The demo-enabled Game process launched and reported Responding=True, but computer-use capture failed with `FrameArrived timed out: timed out waiting on channel`, then `window capture timed out: timed out waiting on channel` after refreshing/recovering the window. No screenshot-backed phase, color, destruction or F4 reload result is claimed. The process started by this run was stopped; settings are byte-identical to the pre-attempt snapshot.

## Narrow support repairs

Fresh builds exposed omitted existing LineDrawer sources/shaders in checked-in graphics project membership and BehaviourTree sources in current test projects. Existing Premake globs already cover them. Test fixtures now use the existing DurationInSeconds field/key, and the application test project no longer compiles nonexistent EnemyComponent.cpp. No scripted-event production behavior changed.

Expected invalid-transform diagnostics exposed an existing fast-process logger shutdown hang after assertions printed PASS. Constant timestamp format literals replaced lazy worker static strings; the identical tests then exited zero with diagnostics enabled. See evidence/logger-shutdown.md.

## EventBus ownership revision

EventBus.hpp and EventBus.cpp are restored to HEAD. The collision caller uses the existing API and its own std::uint32_t subscription storage. Superseded enhancement tests/ticket are retained in superseded/eventbus/, outside active test projects. Fresh Debug solution, CollisionTests and GameApplicationTests build/run pass with the original EventBus. See evidence/eventbus-unchanged-solution-debug.log and evidence/eventbus-unchanged-<suite>-debug-build.log / -run.log. Independent restoration/type-glue reassessments: zero findings. The local GameCollisionDemo.exe is refreshed against the original EventBus; the final main Game.exe is rebuilt with EnableCollisionDemo=false.

## Collider-owned debug outlines

- Debug solution: PASS, evidence/collider-debug-solution.log. Existing Game.cpp C4100 warning remains.
- CollisionTests: PASS, including toggle/color, sphere/box composed transform,
  latest movement, disabled collider visibility, inactive owner and destruction.
  Build log: evidence/collider-debug-tests.log.
- Collision-only public header isolation: PASS Debug, evidence/collider-debug-headers.log.
- EventBus.hpp/.cpp: Git-clean. No staged changes made.
- Visual observation: pending; earlier standalone demo executable predates this extension.
