# General collision detection MVP workflow

Status: in-progress

## Request and authorization

User resumed agp-feature and explicitly authorized source implementation, then resumed after interruption. The design is the general shape-detection spec, with neutral layers and no combat-driven behavior. Staging, commits and remote publication remain unauthorized. Latest user decision: keep the teammate-owned EventBus unchanged; the earlier subscription-safety enhancement is superseded.

## Git and ownership

- Base/HEAD: d627d712a1024b4cfb7f1b623692e51ef4714ae9.
- Integration branch: collision-system, attached checkout; no worktrees or feature commits.
- Initial checkout contained only the untracked collision planning directory; no staged or modified tracked files.
- On resume, Bin/Settings/ApplicationSettings.json contained a pre-existing formatting/current-resolution delta after the interrupted launch. It was excluded from source review. The later runtime attempt preserved the exact pre-attempt settings bytes (snapshot: Intermediate/CollisionRuntimeVerification/ApplicationSettings.before.json).
- Ticket 01 revised owner: root, resolved; EventBus restored unchanged and integration verified. Earlier enhancement record/tests archived under superseded/eventbus/.
- Tickets 02/03 owner: collision_core, resolved.
- Ticket 04 source owner: demo; integrated verification owner: root, in-progress.
- Fresh independent Spec and Standards reviewers reported zero source findings and reassessed subsequent narrow build/test compatibility fixes. See review.md and review-scope.md.

## Current stage

Stage 4: source implementation is complete; EventBus ownership revision is verified; mandatory verification remains incomplete. Debug solution and all four current integrated Debug test suites pass. Earlier EventBus enhancement-only passes are historical; the restored implementation is verified through collision integration. New collision-only headers independently compile in Debug and Release. Main Game and source default are demo-disabled. A separate local demo-enabled Bin/Debug/GameCollisionDemo.exe is prepared for manual observation.

Ticket 04 and the spec remain in-progress because runtime visual/reload observation, Release executable linking, and baseline verification limitations remain unresolved. Build artifacts are continuation aids, not proof of these missing checks.

## Agreed design

- One ColliderComponent per Actor; code-spawned spheres and yaw-only boxes.
- World-owned CollisionSystem, SceneComponent transforms and stable world-local IDs; FindCollider returns borrowed live lookup results.
- Automatic Enter/Stay/Exit per unordered pair after normal updates using frozen frame membership; dispatch from a collected event batch through the existing EventBus.
- One numbered layer (0-31) per collider with bilateral 32-bit masks; default layer 0 and all layers accepted. Gameplay owns meanings.
- Disable/deactivation/destruction/filter changes remove participation on the next detection pass. Clear discards pairs without events and does not reuse IDs.
- Current transform matrices are authoritative, including raw SetMatrix overrides; reject unsupported bases/reflections/shear. Geometry uses double intermediates and a 1e-6 relative touching allowance for float matrix rounding.
- No blocking, navigation, projectiles, capsules, direct queries, importer work, compound colliders or combat behavior.

## Records

- [Spec](spec.md)
- [01 Existing EventBus integration](issues/01-safe-eventbus-lifecycle.md), resolved; no blockers.
- [02 Spheres/events/lifecycle](issues/02-sphere-collider-events.md), resolved; depends on 01.
- [03 Boxes](issues/03-box-collider-overlaps.md), resolved; depends on 02.
- [04 Demo/integrated verification](issues/04-demo-and-verification.md), in-progress; depends on 03.
- [Verification evidence](verification.md)
- [Pending manual visual check](manual-verification.md)
- [Independent reviews](review.md)
- [Historical research](research.md)
- [Retrospective proposals](retrospective.md)

## Verification and remaining limitations

- Final Debug Game.sln build with EnableCollisionDemo=false passes; evidence/final-solution-debug.log.
- Earlier integrated Collision/CustomComponent/ScriptedEvents/GameApplication Debug checks passed. After restoring EventBus, the Debug solution and affected CollisionTests/GameApplicationTests were rebuilt and pass again. Enhancement-only EventBusTests are archived and no longer current requirements.
- All three new collision headers independently compile without backend imports in Debug and Release. Full repository isolation has pre-existing failures; its baseline is retained separately.
- Both source review axes: zero findings, including Windows max-macro and existing DurationInSeconds/test-membership compatibility corrections.
- Release Game and CollisionTests executable links fail LTCG C1007/LNK1257. A fresh v145 CommonUtilities Release rebuild did not fix this. The failure remains an unresolved toolchain/prebuilt-library integration gate; the restart notice alone does not conclusively identify a culprit. No toolset/vendor artifact replacement was performed.
- Legacy GameFrameworkTests still uses removed ServiceLocator APIs and missing importer source paths; current regression suites above were used instead. No full legacy-suite pass is claimed.
- Runtime demo launched as process 39996 and responded, but capture failed with FrameArrived timeout and window-capture timeout after recovery. No visual phase/reload pass is claimed. The test process was stopped. A separate GameCollisionDemo.exe is available for the manual check; main Game.exe was rebuilt with the switch false afterward.
- Narrow support repairs: existing LineDrawer source/shaders and BehaviourTree test-project membership; test DurationInSeconds field/key updates; removal of nonexistent EnemyComponent test compile item; Windows-PowerShell-compatible isolation script; constant Logger timestamp formats to eliminate measured fast-process shutdown hang. No gameplay behavior beyond the collision feature was added.

## Next action

Obtain the observations listed in manual-verification.md and investigate the Release LTCG environment/dependency gate without changing the agreed feature behavior. Recheck Git before continuing; preserve all existing work and the settings delta. Do not declare ticket 04/spec resolved while mandatory checks remain unavailable. No staging or commits without explicit user instruction.

## Latest scope revision

EventBus.hpp restored byte-for-byte from HEAD; EventBus.cpp was already unchanged. Collision-owned subscription members use std::uint32_t, and the demo implementation includes EventBus.hpp only where its methods are called. No subscription-safety implementation is part of collision delivery. Debug solution plus CollisionTests/GameApplicationTests pass with the original EventBus; main/demo artifacts are refreshed against it. Preserve unrelated CODING_STANDARDS.md and settings changes present at this revision start.

## Collider debug drawing extension

Requested per-component drawing controls are implemented: SetDebugDrawEnabled,
SetDebugColor and SetDebugDisabledColor, with matching getters. Default drawing
is off. World queues outlines after gameplay updates and overlap callbacks;
disabled colliders draw with the disabled color, inactive/destroyed objects do
not draw. The demo now configures component settings instead of drawing geometry
itself. EventBus remains unchanged.

Debug Game.sln build passes (existing Game.cpp unused aWorld warning remains).
CollisionTests passes with toggle/color, sphere/box composed transform, current
frame movement, inactive owner and destruction coverage. Collision-only Debug
header isolation passes; git diff --check passes. Evidence: collider-debug-*.log.
Visual appearance is not yet observed; the earlier standalone GameCollisionDemo.exe
predates this extension and must be rebuilt before verifying these new controls.
Existing Release and manual verification gates above remain open.
