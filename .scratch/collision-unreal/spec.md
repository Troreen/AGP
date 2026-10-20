# Unreal-authored collision triggers

Status: in-progress

## Problem Statement

AGP detects code-authored overlaps and can start scripted sequences, but exported
Unreal shapes still become placeholders. Designers cannot yet author a complete
collision encounter in Unreal and run it in Game.

## Solution

First prepare a small Perforce checkpoint of the reusable collision implementation,
excluding the optional demo. Then deliver a box-trigger round trip, followed by
sphere parity, using actual exporter evidence before selecting shape semantics.
The user resumed implementation after committing the existing working-tree
changes. Work in the component test scene and retain the existing exporter
contract. Do not submit to Perforce. Keep implementation increments reviewable.

## User Stories

1. As a teammate, I can review and later sync the collision foundation separately
   from Unreal integration.
2. As a designer, I can place a box trigger in Unreal and see its matching outline
   in AGP, including its actor-relative offset and supported yaw/scale.
3. As a player, entering that volume starts its scripted sequence once and leaves
   a disabled gray outline while the sequence completes.
4. As a designer, I can use sphere volumes with the same behavior.
5. As a developer, I receive useful diagnostics for unsupported or ambiguous
   exports rather than silently receiving a different collision shape.
6. As a tester, I can reload the scene and repeat the encounter without retaining
   old subscriptions or consumed-trigger state.

## Implementation Decisions

- Keep importer conversion separate from runtime construction: importer returns
  owned scene data; the builder constructs components and applies common state.
- Confirm shape units, axes, size interpretation, hierarchy flattening, and whether
  dimensions already include scale from the actual exporter and real fixtures.
  Existing parsing reorders box bounds and applies a unit factor; this is evidence
  of current behavior, not proof of the exporter contract.
- Import only the existing shape and common component fields described in the
  exporter programmer guide. Keep the exporter unchanged. Shape component tags
  CollisionLayer=<name> and CollisionMask=<name>|<name> configure AGP filtering.
  Only Player=1, Trigger=2 and Enemy=3 are named in game policy. The importer
  receives a name-to-layer map; engine code does not hardcode gameplay names.
  Missing tags retain enabled=true, layer 0 and all mask bits; an explicitly
  empty CollisionMask accepts none. Unknown names, malformed settings and duplicate
  setting tags fail import with clear diagnostics. Tags remain source metadata.
- Keep the current one-collider-per-Actor rule. Define deterministic diagnostics
  for multiple shapes on one Actor before implementation; do not silently pick one.
- Start with a box volume, a code-created Player sphere, and a simple existing
  movement/visibility sequence. Measure Player collider radius and offset against
  the mesh and scene; avoid a duplicate exported Player Actor.
- User narrowed the current increment to import and tags first. Existing Player
  creation is disabled for shader testing and stays unchanged. Restoring Player
  and proving full sequence activation belong to the following increment.
- The sequence's optional exported TriggerCollider string names a collider on
  the same Actor. No separate trigger component or EnteringActor name filter is
  needed; collision layers/masks select eligible entrants. Missing/wrong-type
  names produce diagnostics. Successful collision Play disables that collider;
  rejected Play permits a later entry. Stay and Exit do not activate playback.
- Sphere parity retains positive uniform scale; boxes retain positive scale and
  world-up yaw. Unsupported transforms remain excluded from detection.

## Testing Decisions

- Prefer real exported JSON fixtures through the public import-to-BuildWorld path,
  then observe World updates and collider callbacks. Verify behavior and geometry,
  not private helper structure. Use existing framework/custom-component tests as
  prior art; add narrow collision-import coverage where fixtures fit best.
- Test translated/offset and yawed boxes, uniform spheres, retained defaults,
  and duplicate/unsupported records at public seams. Runtime enablement and
  filtering remain covered separately by existing collision regressions.
- Preserve collision and scripted-event regressions covering initial overlap,
  rejected playback followed by successful re-entry, disablement, destruction,
  and scene replacement.
- Build Debug x64 Game and affected test projects. Run public-header isolation if
  scene/collider public interfaces change. Release linking has a historical
  unresolved toolchain gate; recheck and report the current result explicitly.
- Visual acceptance uses a named exported test scene and the normal Player:
  correct volume placement; unrelated Actor ignored; entry log; sequence completes;
  consumed volume gray; no retrigger; F4 recreates the scene and permits one new
  activation. Repeat with initial overlap and rejected playback/re-entry.

## Out of Scope

Blocking movement, physics, capsules, compound colliders, swept detection,
animation-import completion, exporter changes, native Unreal collision settings,
automatic Git commits, and Perforce submission.
No shipping demo Actors or new demo switches.

## Further Notes

Actual exported SE_WorkflowTest/TriggerBox establishes the box fixture. Source
references, expected geometry and continuation evidence live in export-contract.md,
workflow.md and verification.md. Runtime and sphere export evidence remain explicit
verification gates rather than inferred from source or synthetic fixtures.
