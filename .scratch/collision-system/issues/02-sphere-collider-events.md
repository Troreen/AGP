# 02: Spawn sphere colliders and receive events

Status: resolved
Blocked by: 01

## Parent and implementation gate

[Collision detection MVP spec](../spec.md), criteria T2-T4. Read [workflow](../workflow.md): source implementation is authorized by the resume request; see workflow.md.

## What to build

An end-to-end sphere collider path: spawn Actor, add/configure one ColliderComponent, advance World, and receive typed Enter/Stay/Exit through the existing shared EventBus. Include filtering and lifecycle safety in this slice, not a geometry-only layer.

## Acceptance criteria

- [x] ColliderComponent derives from SceneComponent; configure a positive finite sphere radius and local transform using the spec's API.
- [x] Reject a second non-destroyed ColliderComponent on an Actor, even if either is disabled. A replacement gets a fresh identity.
- [x] World owns CollisionSystem; gameplay does not manually register colliders.
- [x] IDs are assigned on attachment, nonzero, world-local, and not reused during that World object's lifetime. FindCollider returns a borrowed pointer or null for missing/pending-destroy IDs.
- [x] Sphere overlap uses composed current transforms, positive uniform scale, and inclusive touching.
- [x] Enforce one numbered layer (0-31) per collider and a bilateral 32-bit detection mask; default layer 0 and all layers allowed. Reject out-of-range layers without changing prior state; accept zero masks.
- [x] Detection uses existing frozen frame membership, begun/enabled state, and active owners after Component updates.
- [x] Each unordered overlapping pair produces exactly one Enter on its first pass and one Stay on subsequent passes; removal produces one Exit.
- [x] Detect independent simultaneous occupants using actual ID-pair equality.
- [x] Disable, deactivation, filter change, destruction, re-enable, and replacement follow spec semantics. Cleanup never requires dereferencing a destroyed participant.
- [x] Event payload contains source World, ordered collider IDs, and phase. Batch detection completes before dispatch; callback mutations affect the next pass.
- [x] Clear discards pairs without teardown events, preserves ID monotonicity, and retains World lifecycle guards.
- [x] Invalid dimensions preserve prior valid configuration; unsupported transforms are diagnosed and excluded rather than approximated.

## Verification

Behavioral tests through World, ColliderComponent, and EventBus: Enter/Stay/Exit, tangency, offsets/scale, default masks, layers 0/31, one-sided mask rejection, zero masks, invalid layers, filter-change Exit/re-entry, multiple occupants, repeated zero-delta updates, BeginPlay/update spawning, callback destruction, disable/deactivate, re-enable, FindCollider, and Clear. Demonstrate listener setup/cleanup in a small test Component. Run the affected framework tests and public-header isolation; update source membership in both project files and Premake conventions where required.

## Comments

Use a simple pair loop and internal geometry helpers; no physics dependency, spatial tree, global registry, compound support, damage, timers, importer changes, or public explicit query API. Detection behavior is independent of consuming gameplay systems.

## Verification evidence

Sphere slice verified through public World/ColliderComponent/EventBus APIs. See evidence/02-red-build.log (new API absent), evidence/02-transform-red.log (canceling raw reflections initially accepted), evidence/02-lifecycle-build.log and evidence/02-lifecycle-run.log (Debug build and complete sphere/lifecycle pass). Tests cover T2-T4, filtering, simultaneous occupants, frozen spawn membership, live transforms, replacement IDs, callback mutation/exception recovery, and Component subscription cleanup. New collision-only public header isolation passes: evidence/02-header-isolation.log. Whole-repo header isolation has pre-existing failures: evidence/header-isolation-baseline.log. A fast diagnostic-producing test initially hung at process teardown after PASS; a narrowly authorized Logger::Timestamp literal-format change removed worker lazy static string initialization and the same tests now exit successfully with production warnings enabled. No staging or commits. Premake already recursively includes new framework sources; checked-in project/filter membership updated.
