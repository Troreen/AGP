# Scripted events
Status: in-progress

## Problem Statement
Independent staged encounters need reusable actions authored as custom components in Unreal and executed in AGP. Unreal is unavailable today; its editor setup will be done tomorrow.

## Solution
One dedicated Actor holds a ScriptedSequenceComponent and finite reusable action components. A named input action starts a sequence now; Play() supports future trigger boxes.

## User Stories
1. Author independent events using action components and numbered steps.
2. Start all actions in a step together and wait only for selected actions.
3. Play registered animation clips with temporary tree override or set tree bools/triggers.
4. Play VO, music, and optionally positional SFX, waiting for the registered sound to finish when selected.
5. Move an Actor directly to a named marker over a duration.
6. Lock/unlock player commands explicitly, clearing existing movement while camera follow continues.
7. Activate Actors and enable/disable named behavior components to reveal objects and hand enemies back to AI.
8. Default to one run per scene load; optionally replay after completion. Ignore triggers during playback.
9. Reject conflicting sequences before applying changes.
10. Keep authored positions and activation changes; release temporary movement, input, and animation control on finish/cancel.
11. Set up and demonstrate the runtime without Unreal; receive exact export settings for later editor authoring.

## Implementation Decisions
- Explicit per-frame component runner; do not depend on the unfinished coroutine scheduler.
- Shared fields: Step, WaitForCompletion, TargetActor, TargetComponent. Equal steps use component attachment order for immediate writes; distinct steps communicate dependencies.
- Actions: Wait, Move, PlayAnimation, SetAnimationVariable, PlayAudio, LockControl, UnlockControl, SetActorActive, SetComponentEnabled.
- Targets and markers use exported unique Actor names. Resolve borrowed pointers by name each time to survive target destruction.
- Claim all controlled target Actors at start until finish/cancel; sound sources and markers are read-only and unclaimed.
- Rejected start leaves replay eligibility intact. Accepted/cancelled once-only runs remain consumed.
- Invalid configuration fails before side effects; missing objects during playback cancel safely.
- Mesh components stay alive while their animators use them; separate runtime mesh removal is outside the supported lifecycle. Validate mesh/clip setup before sequence playback, without per-frame component searches.
- Direct animation supports nonloop completion and looping without wait; bool/trigger actions complete immediately rather than guessing tree state completion.
- Releasing a direct clip restarts the tree's current animation at frame zero without entering/exiting the state again or resetting variables. Without a tree, hold the scripted pose until another animation is requested. No saved playback snapshot.
- Non-wait actions continue into later steps but movement/animation stop on sequence finish. Started audio can continue after normal completion, stops on cancel, replay, action destruction, and scene unload.
- Audio uses one existing registered playback per SoundID. No script-specific instance pool, handle ownership, reuse flags, or Release API. Actions start/check/stop by SoundID; independently controlled overlapping copies of an ID are outside the intended use.
- Input lock affects gameplay commands, not Escape, reload, or debug controls. Movement action suppresses PlayerMovement while active; other AI is explicitly disabled using actions when needed.
- Default demo is manual F2 with replay enabled; trigger boxes, camera control, new effects, Unreal plugin/editor modifications, and event-specific content production are out of scope.

## Testing Decisions
Public seams: World/Actor component lifecycle, sequence Play/Cancel/status, real actions, custom-component import/build, and existing player/animation/audio interfaces. Verify ordering, waits, movement, replay/conflicts, cancellation and destruction through public behavior. Build Game plus relevant framework tests with v145. Runtime checks cover F2, camera follow, control restoration, animation/audio, reload and exit; automated logic tests do not establish visual/audio correctness.

## Out of Scope
Unreal editor authoring/round trip is pending tomorrow. No trigger-box implementation, camera actions, navigation, destruction simulation, dedicated particle action, or general message bus.
