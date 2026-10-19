# 02: Register and load EnteringActor custom configuration

Status: in-progress
Blocked by: 01

## Spec

../spec.md; AC7-AC8.

## What to build

Register `CollisionScriptTriggerComponent` and test custom component creation with the existing registry/builder path. The trigger accepts one flattened custom string named `EnteringActor`. No importer shape or exporter schema changes are part of this work.

## Acceptance criteria

- [ ] Registry creates the exact component through the supported custom type name.
- [ ] Builder applies the exported EnteringActor string to a code-authored component instance.
- [ ] With collider geometry attached in code, the built trigger activates an observable action through a real World overlap.
- [ ] Game and relevant checked-in test Visual Studio projects include the source; Premake's current glob includes it without edits.
- [ ] Existing direct sequence activation remains available.

## Verification

The registry/builder fixture creates the component from `CollisionScriptTriggerComponent_C` with an EnteringActor string, attaches code-authored collider geometry, and asserts movement plus collider disablement. ScriptedEventsTests now passes with this coverage. No exporter changes were made. The final Game.sln and CustomComponentTests Debug x64 builds and executables succeeded. The scripted tests verify the registered custom component load and EnteringActor round trip.