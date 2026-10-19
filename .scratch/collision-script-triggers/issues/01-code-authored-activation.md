# 01: Activate the same Actor's sequence from a collider Enter

Status: in-progress
Blocked by: none

## Spec

../spec.md; AC1-AC6.

## What to build

Add `CollisionScriptTriggerComponent` with the single `EnteringActor` custom string. Its Actor owns the one collider, sequence coordinator, and actions. The trigger subscribes during BeginPlay, accepts only Enter involving its collider and the configured Actor, calls Play, and disables the collider only after successful Play. Failed Play preserves the collider for a later entry.

## Acceptance criteria

- [ ] Box/sphere World overlap dispatch starts an observable same-Actor sequence for matching entry, including initial overlap and either collider ID ordering.
- [ ] Other Actors, events unrelated to the source collider, Stay, and Exit do not activate.
- [ ] Successful Play consumes the trigger by disabling its collider; rejected Play leaves it enabled, Stay does not retry, and a new Enter can.
- [ ] Invalid component configuration logs context and remains inert.
- [ ] BeginPlay/EndPlay subscriptions clean up through Clear and replacement World.
- [ ] Existing manual `ScriptedEventComponent::Play()` behavior remains intact.

## Verification

The final Debug x64 Game.sln build succeeded with MSBuild v145 using serial compilation (`/m:1 /p:MultiProcessorCompilation=false /p:CL_MPCount=1`). ScriptedEventsTests, CollisionTests, and CustomComponentTests each built and ran successfully. ScriptedEventsTests stdout/stderr are captured in `../test.stdout.txt` and `../test.stderr.txt`. The fixture initializes ServiceLocator before World BeginPlay and clears the World before service teardown. The opt-in runtime startup smoke also succeeded; visual entry/action/reload behavior remains for human GUI verification.