# Collision triggers from Unreal

## Ready now

Put a collider, one `ScriptedEventComponent`, and the sequence actions on the same
Actor. Set the sequence's exported String field `TriggerCollider` to that collider's
component name, for example `TriggerBox`. No separate trigger component is needed.
Leave the string empty for manual-only activation. A missing or wrong-type named
component produces a diagnostic; another collider is never selected implicitly.

Entry starts the sequence. Success disables the collider and logs both Actor
names; the sequence keeps running. Failure leaves the collider enabled for a
later entry. Debug Game builds show collider outlines; disabled ones are gray.
The temporary scripted demo has been removed.

The importer now maps exported Box and Sphere components to runtime colliders.
It reads existing geometry, transforms, and Tags; no exporter change is required.
See [shape import and filtering](CollisionDetection.md#import-shapes-from-unreal)
for the supported attachment rules. Capsules remain placeholders.

`ScriptedEventComponent::BeginPlay` sets its named collider to layer Trigger
and mask Player. Player creation sets layer Player and mask Trigger in C++.
No collision Tags are needed in Unreal; existing collision Tags are ordinary
metadata and do not configure filtering. Other gameplay classes can choose
their own layers with `SetLayer` and `SetOverlapsWith`. Admission depends on these
settings rather than Actor names. Native Unreal collision responses are not exported.

The supplied component test export has `SE_WorkflowTest/TriggerBox` with local
half-extents `{32,32,32}` and scale about `8.765`. Its expected AGP center is
`{-130,254,-1560}` and world half-extents are about `{280.48,280.48,280.48}`.
These values provide an alignment target. The user has verified entry, the gray
consumed outline and F4 reset in Debug Game after a real TGA re-export.

## Verified component test encounter

The sequence handles its Actor's `OnCollisionEnter` method directly. Configure
`SE_WorkflowTest/ScriptedEventSequence` with `TriggerCollider=TriggerBox` and remove
the obsolete `CollisionScriptTrigger` instance when migrating older maps. Export
the updated map through the normal TGA workflow. The exporter has not been modified.
Action setup follows the [existing guide](ScriptedEventsUnrealTestGuide.md).

Player creation is restored in `Game.cpp` with radius 60, local offset `{0,90,0}`,
layer Player and mask Trigger. Do not export another Actor named Player.
Before this refactor, the user confirmed F2 sequence completion, automatic activation on first entry,
gray consumption, and F4 restoring green and allowing activation again. The
sequence's `Replayable=true` permits manual replay; the collision trigger consumes
its collider on the first accepted collision start. Re-entry alone does not rearm it.

A focused fixture from the updated real export covers import, registration, Player/Enemy
filtering, entry, completion and consumption. Existing tests cover initial overlap
and retry after rejected playback. Exact Player body fit and a real exported
sphere's visual alignment remain future checks; the Player sphere is approximate.
The sequence-owned trigger's fresh export is validated; its human entry/gray/F4
check is pending.

## Other Unreal checks still open

- Animation targets need runtime animator, mesh, clips, and tree/variable setup.
  Full Unreal animator import is separate work; the existing guide uses a C++
  target. Complete that path if targets must be fully authored in Unreal.
- Run the existing guide's visual/audio action checks with the team's actual
  Unreal project and assets. Automated tests do not verify editor export.

These are nonblocking overlaps, not walls or physics. See
[collision limits](CollisionDetection.md) and [importer handoff](ImporterHandoff.md).
