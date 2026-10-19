# Collision triggers: Unreal next steps

## Ready now

Put one `ColliderComponent`, one `CollisionScriptTriggerComponent`, one
`ScriptedEventComponent`, and the sequence actions on the same Actor. Set the
trigger's exported String field `EnteringActor` to the exact entering Actor name,
for example `Player`.

Entry starts the sequence. Success disables the collider and logs both Actor
names; the sequence keeps running. Failure leaves the collider enabled for a
later entry. Debug Game builds show collider outlines; disabled ones are gray.
The temporary scripted demo has been removed.

## Still to do

1. **Import real colliders.** Box/Sphere/Capsule exports currently become
   placeholders in `SceneData`, not runtime colliders. Agree the mapping with the
   Unreal/exporter owner, then update `SceneWorldBuilder` and importer data as
   needed. Start with boxes and spheres; capsules are not supported by detection.
   Carry names, enabled state, relative transforms, radius/half-extents, layers,
   and masks. Confirm units, axes, and scale with a real export. Boxes support
   yaw and positive scale; spheres require uniform positive scale.
2. **Give Player a collider too.** The normal C++ Player currently has none.
   Configure it there if Player remains code-created; avoid exporting another
   Actor with the same name. Both collider masks must accept the other's layer.
3. **Author the trigger in Unreal.** Create the custom Actor Component Blueprint
   `CollisionScriptTriggerComponent` with String field `EnteringActor`. Add it,
   the collision shape, coordinator, and actions to one trigger Actor. Configure
   action targets, steps, and timing using the
   [existing guide](ScriptedEventsUnrealTestGuide.md).
4. **Verify the export round trip.** Check the custom type and `EnteringActor`
   survive export. In Game, check volume placement, unrelated Actors, entry,
   activation log, gray outline, action completion, no retrigger, and F4 reload.
   Also test initial overlap and retry after rejected playback.

## Other Unreal checks still open

- Animation targets need runtime animator, mesh, clips, and tree/variable setup.
  Full Unreal animator import is separate work; the existing guide uses a C++
  target. Complete that path if targets must be fully authored in Unreal.
- Run the existing guide's visual/audio action checks with the team's actual
  Unreal project and assets. Automated tests do not verify editor export.

These are nonblocking overlaps, not walls or physics. See
[collision limits](CollisionDetection.md) and [importer handoff](ImporterHandoff.md).
