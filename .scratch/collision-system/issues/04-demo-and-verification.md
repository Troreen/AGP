# 04: Demonstrate and verify the collision detection MVP

Status: in-progress
Blocked by: 03

## Parent and implementation gate

[Collision detection MVP spec](../spec.md), criteria T6-T7 and combined T1-T5 regression verification. Read [workflow](../workflow.md): source implementation is authorized by the resume request; see workflow.md.

## What to build

An opt-in, code-spawned runtime demonstration and short usage documentation showing a gameplay Component receiving collider events and cleaning up its subscription. Verify the integrated MVP and review it against the spec and repo standards.

## Acceptance criteria

- [x] One named code debug boolean, disabled by default, enables a demonstration through the existing Game::ConfigureWorld path.
- [x] Spawn separate Actors with sphere/box colliders, reuse LineDrawer for outlines, and move a volume slowly through stationary volumes; use neutral detection examples and demonstrate layer/mask filtering.
- [x] A sample gameplay Component subscribes in BeginPlay, filters by collider ID, logs Enter/Stay/Exit, and unsubscribes in EndPlay.
- [ ] Exercise overlap, separation, enable/disable, and destruction visibly, without combat rules, new settings formats, or permanent input bindings.
- [x] Document spawn/configure/subscribe/unsubscribe usage, frame-sampled limitations, one-collider restriction, and borrowed lookup lifetimes.
- [ ] Record exact executable, scene, opt-in switch, actions, and expected phases for the runtime check; remove listeners on scene reload.
- [ ] Debug/Release x64 solution builds and relevant framework regression executables pass; public-header isolation passes for new public headers.
- [x] Review the same selected feature scope against the original base for spec compliance and CODING_STANDARDS. Use fresh independent reviewers if available; otherwise disclose separate self-review passes.
- [x] Resolve confirmed in-scope findings, rerun affected checks, and record remaining limitations without claiming unobserved checks passed.

## Verification

Follow Docs/AgentWorkflow.md for v145 builds, direct test-project builds, executable paths, and header isolation. Relevant projects include GameFrameworkTests, GameApplicationTests, CustomComponentTests, and ScriptedEventsTests where lifecycle changes affect them. Inspect the runtime demo and reload behavior. Do not invent performance or swept-detection guarantees.

## Comments

Dependencies are transitive: 03 requires 02, which requires 01. Runtime checks that require human evidence keep this ticket in-progress until completed. No staging or commits without explicit user instruction.

## Verification evidence

Demo and usage documentation implemented. Final Debug solution and Collision/CustomComponent/ScriptedEvents/GameApplication regression results are recorded in [verification](../verification.md). Independent source reviews found zero issues, including reassessment of small compile/test compatibility fixes. New collision headers pass isolated Debug/Release compilation. Main Game/default source are restored to demo-disabled; a separate local GameCollisionDemo.exe is prepared for the pending human check.

Required runtime/reload observations remain unverified after two computer-use frame capture failures. Release executable links remain blocked by unresolved LTCG C1007/LNK1257; rebuilding CommonUtilities did not solve it. The pre-existing whole-repo isolation and legacy GameFrameworkTests failures remain disclosed. See [manual check](../manual-verification.md) and [review](../review.md). Keep this ticket and spec in-progress until required gates are satisfied. No staging or commits.

Latest user decision keeps EventBus unchanged; affected Debug solution, CollisionTests and GameApplicationTests pass after restoring it. Enhancement-only EventBus tests are historical and outside current acceptance.
