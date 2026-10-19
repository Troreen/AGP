# Collision activation of scripted events

Status: in-progress

## Problem

Code-authored colliders publish overlap events, but entering a volume cannot yet activate an authored scripted sequence. Add a reusable game component that connects one collider to one sequence on the same Actor.

## User-authorized scope

Add `CollisionScriptTriggerComponent`, derived from `CustomComponent`, to an Actor that also owns exactly one `ColliderComponent`, exactly one `ScriptedEventComponent`, and that sequence's action components. Its sole setting is the required string `EnteringActor`. Subscribe to overlap events in `BeginPlay`; unsubscribe in `EndPlay`. An Enter involving its own collider activates only when the other collider belongs to an Actor whose name equals `EnteringActor`. Call the sequence coordinator's `Play()`. Disable the source collider only when Play succeeds. The Actor stays active. Failed Play leaves the collider enabled; another Enter may try again. Stay and Exit do nothing. Initial overlap produces Enter under existing collision semantics.

Assume one active World; do not add world filtering. Preserve existing sequence manual activation. Do not add replay controls, consumed flags, occupancy logic, imported shape support, exporter changes, multiple-collider support, EventBus changes, or normal player setup changes. Add an opt-in controlled runtime demo that is disabled by default.

## Acceptance criteria

- AC1: Matching Player (or configured Actor) Enter starts the sequence on the collider owner's Actor.
- AC2: Other Actor names, events not involving the owner's collider, Stay, and Exit do not start it.
- AC3: A successful Play disables the collider and the Actor remains active; later overlap passes do not activate it again.
- AC4: A rejected Play leaves the collider enabled; Stay does not retry, while a new Enter can succeed after the sequence becomes valid.
- AC5: Missing sibling collider/coordinator or empty EnteringActor yields a contextual diagnostic and inert component.
- AC6: BeginPlay/EndPlay subscription lifetime is safe through World Clear and scene replacement.
- AC7: Registry and builder create the component and apply the EnteringActor custom string. Existing manual sequence activation remains available.
- AC8: Checked-in Visual Studio source membership is correct and Premake's existing source glob includes the new files.
- AC9: The documented opt-in runtime demo visibly demonstrates entry activation and sequence output; its switch remains false by default and normal Player configuration is unchanged.

## Test seams

Use real World updates, ColliderComponents, EventBus dispatch, ScriptedEventComponent, and observable action results. Cover both collider ID orders, initial overlap, unrelated Actor filtering, source identity, successful consumption, rejected Play retention, Stay vs. fresh Enter retry, custom registry/builder field loading, and cleanup/replacement. Existing direct `ScriptedEventComponent::Play()` tests preserve manual activation. Runtime demo checks entry, action result, F4 reload, and the off-by-default setting.

## Verification

Build Debug x64 Game.sln and the relevant ScriptedEventsTests, CollisionTests, and CustomComponentTests projects. Run those test executables, the public-header isolation script if the public gameplay header requires it, and the opt-in runtime procedure when the graphical environment permits. Record exact commands, results, and limitations below and in the corresponding tickets. Do not stage or commit.