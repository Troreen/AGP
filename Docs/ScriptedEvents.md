# Scripted events

A scripted event is a dedicated Actor with exactly one `ScriptedEventComponent` and any number of action components. Each encounter has its own Actor. Actions are reusable custom component types; an event does not need a new C++ class.

## Playing an event

The coordinator exposes `Play()`, `Cancel()`, `IsRunning()`, `HasCompleted()`, and `GetLastError()`. A future trigger box can call `Play()` on it. For now, set `TriggerAction` to an existing input action name. The current input settings bind `PlayUnrealSequenceTest` to F2. An exported coordinator with that TriggerAction runs when the key is pressed; no event is created automatically in C++.

`Replayable` defaults to `false`: a successfully started event can run once per scene load, including if it was later cancelled. Set it to `true` for repeated testing. An event cannot restart while running. Invalid configuration or a claim conflict rejects playback before actions start and does not consume the attempt.

Actions run in ascending `Step` order. Equal steps start together; use unique component names. The next step starts when all actions in the current step with `WaitForCompletion=true` finish. Other started actions continue updating across steps. Finishing the last step ends the sequence, so keep a waiting action alive for as long as scripted movement or animation is needed. Step numbers may have gaps. Equal-step actions start in component attachment order; put dependencies in different steps. Overlapping movement or direct-animation actions on the same Actor are rejected; a non-waiting action on one of these channels must be the last such action for that Actor in the sequence.

## Component fields

Names and variable types must match exactly. Put each variable into its corresponding Unreal custom variable map: `StringVariables`, `IntVariables`, `FloatVariables`, or `BoolVariables`. The existing exporter flattens these into the exported `Custom` object. These actions require no new exporter value types.

| Component | Fields beyond the common action fields |
| --- | --- |
| `ScriptedEventComponent` | String `TriggerAction` (empty); bool `Replayable` (`false`). This is the coordinator, not an action. |
| `WaitActionComponent` | Float `DurationInSeconds` (`1.0`). No target required. |
| `MoveActionComponent` | String `DestinationActor`; float `DurationInSeconds` (`1.0`). |
| `PlayAnimationActionComponent` | String `Animation`; bool `Loop` (`false`). |
| `SetAnimationVariableActionComponent` | String `Variable`; bool `Trigger` (`false`); bool `Value` (`true`). |
| `PlayAudioActionComponent` | Int `Sound` (`0`); bool `Positional` (`false`). |
| `LockControlActionComponent` | Common fields; empty `TargetActor` defaults to `Player`. |
| `UnlockControlActionComponent` | Common fields; empty `TargetActor` defaults to `Player`. |
| `SetActorActiveActionComponent` | Bool `Active` (`true`). |
| `SetComponentEnabledActionComponent` | Bool `Enabled` (`true`); requires `TargetComponent`. |

Every action accepts int `Step` (`0`), bool `WaitForCompletion` (`true`), string `TargetActor` (empty), and string `TargetComponent` (empty). Target names are runtime Actor/component names, not asset paths. Use unique Actor names. Actions target another Actor, never the sequence Actor itself. Where supported, an empty `TargetComponent` selects the first matching component on the target Actor; set it explicitly when there are several animators or player controllers. A component name refers to its instance name, not its custom type.

Durations must be finite and nonnegative. Zero duration completes on the next sequence update. Movement interpolates world position directly to the named marker's position captured when the action starts; it does not navigate around obstacles or set facing. Use a marker Actor with the desired exported world position. Movement temporarily suspends the built-in PlayerMovementComponent; disable other movement/AI components explicitly when they could fight the scripted transform.

`Animation` is an alias already registered on the target `AnimatorComponent`, for example `Idle`; it does not load an animation asset. Register new clips in the actor's existing animation setup before authoring the event. The animator's mesh component stays attached during runtime; removing it separately is not supported. A direct clip temporarily overrides normal animation and releases that override when it finishes or the sequence ends. Keep the target Actor active and its animator enabled while waiting for a clip. A looping clip requires `WaitForCompletion=false` and another waiting action to hold the event open.

When a direct clip releases control, the animation tree restarts its current state's clip from frame zero. The state and its variables stay unchanged, and entry/exit events do not run again. Without an animation tree, the actor holds its scripted pose until another animation is requested. Cancelling also uses this handoff; it does not restore a saved animation or playback time.

Animation variables support bools and triggers. With `Trigger=false`, the action calls `SetBool(Variable, Value)`; with `Trigger=true`, it calls `SetTrigger(Variable)`. The variable must already exist in the target's animation tree. These actions finish immediately; they do not wait for the resulting animation transition. Use a direct clip or explicit wait when timing is required.

`Sound` is the integer value of an existing `SoundID` in `AudioManager.h`, not an FMOD path or Unreal sound reference. New VO/music must first be registered in AGP's audio setup. `Positional=true` requires `TargetActor` and starts audio at its current world position; it does not follow a moving Actor. Each SoundID uses its existing registered playback. Two scripted events using the same ID share control of that sound, so do not use the same ID for independently controlled overlapping audio (including a background music playback using that ID). Waiting audio completes when FMOD reports that registered sound has finished. Do not wait on looping ambience/music. Non-waiting audio may continue after normal sequence completion; cancellation, scene teardown, and replay stop the registered sounds started by that sequence.

Control locking clears existing player movement and blocks gameplay input while camera follow continues. Unlock explicitly with an `UnlockControlActionComponent` for an earlier handoff, or let sequence cleanup release its lock. Scripted movement remains possible while control is locked.

`SetActorActiveActionComponent` and `SetComponentEnabledActionComponent` leave their changes in place. To keep an enemy passive until a reveal, disable its behavior component before it can update, then enable that named component in the handoff step. Setting the whole Actor inactive also hides it, so use component enabling when the enemy should stay visible. The sequence does not implement combat, destruction, or AI states itself.

All non-audio target actions reserve their target Actors for the whole event. A second sequence targeting any reserved Actor is rejected rather than interrupting it. Positional audio does not reserve its source. Cancelling, completing, or unloading the scene releases claims and temporary movement/animation/control overrides. Completed position changes, animation variables, activation, and component enabled states remain authored results; cancellation does not rewind them.

## Examples to author

These are separate events, each on its own sequence Actor. Clip aliases, audio IDs, and Actor/component names below must be replaced with names available in the scene.

| Event | Step | Actions |
| --- | --- | --- |
| Heart episode | 0 | Lock `Player`; play a registered heart animation on `Player` and wait; play heart audio with `WaitForCompletion=false`. |
| Heart episode | 1 | Unlock `Player`. Normal completion also releases the temporary animation override. |
| Bridge | 0 | Play registered VO. Use waiting audio if the player should keep moving until the VO finishes; use non-waiting audio if the next step should begin immediately. |
| Bridge | 1 | Lock `Player`; play the registered heart animation and wait. |
| Bridge | 2 | Unlock `Player`. |
| Popcorn reveal | 0 | Lock `Player`; move `Player` to marker `PopcornApproach` over an authored duration and wait. |
| Popcorn reveal | 1 | Play the enemy reveal clip and wait; optionally play positional audio on the enemy without waiting. |
| Popcorn reveal | 2 | Enable the enemy's named behavior component; unlock `Player`. |

The distant crying and close reveal can use independent sequence Actors. Trigger boxes and distance detection are future callers of `Play()`; this version supplies the manual input entry point. Dedicated camera and VFX actions are also outside this version. Existing effect Actors can be activated with an activation action.

## Unreal setup tomorrow

Unreal is unavailable during this implementation. The editor setup and a real editor export remain unverified until the following steps are done.

1. Follow the existing exported custom-component pattern used by `SpinComponent`. Create finite custom component blueprints/types with the exact `CustomType` names in the table. Set up the matching typed variable maps and defaults. Verify the exported custom type string; a Blueprint asset name alone is not proof that it matches.
2. Place a dedicated Actor for each event. Add one coordinator and uniquely named action instances. Keep the sequence Actor active and its coordinator enabled throughout playback.
3. Name the target Actors, animator/behavior component instances, and marker Actors clearly. Confirm target animation aliases and bool/trigger names in AGP. Prepare inactive/disabled enemy behavior through the existing scene/runtime setup before gameplay starts.
4. Give the test coordinator `TriggerAction=PlayUnrealSequenceTest` and `Replayable=true`. Export a small scene and inspect its component records against the example below. Existing scalar maps already cover all action fields; no exporter format change is required.
5. Run AGP from its normal runtime directory and press F2 to play the exported test sequence. Follow the [Unreal test guide](ScriptedEventsUnrealTestGuide.md) for movement, replay, and scene replacement checks, plus optional checks for the remaining actions.

The following is a complete custom component record within an exported Actor's `Components` array. The surrounding Actor/level records are still required. The exporter supplies the normal parent and transform values; the identity matrix below is only an example.

```json
{
  "Name": "MovePlayerToMarker",
  "Parent": "",
  "TypeID": -1,
  "Tags": [],
  "Transform": [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1],
  "CustomType": "MoveActionComponent",
  "Custom": {
    "Step": 0,
    "WaitForCompletion": true,
    "TargetActor": "Player",
    "DestinationActor": "PopcornApproach",
    "DurationInSeconds": 2.0
  }
}
```

For the coordinator, use the same component envelope with `CustomType="ScriptedEventComponent"` and `Custom={"TriggerAction":"PlayUnrealSequenceTest","Replayable":true}`. The importer reads flattened scalar values directly under `Custom`; do not put nested `StringVariables` or `FloatVariables` objects there. For hand-written JSON, keep float values such as `DurationInSeconds` written as `2.0`, and ints such as `Step` and `Sound` written as `2`: the importer rejects a variable with the wrong type.

## Manual checks

Use the [Unreal test guide](ScriptedEventsUnrealTestGuide.md) to author and export a test sequence. The built-in C++ demo has been removed. Audio and animation checks are optional follow-up work and remain unverified in the editor round trip until performed.

## Verification and extending actions

Build `Game.sln` in Debug x64, then build `Tests/GameFramework/ScriptedEventsTests.vcxproj` and run `Bin/Tests/Debug/ScriptedEventsTests.exe`. These tests exercise real components and CPU animation assets; they do not establish audible/visual playback or an Unreal editor round trip. The current run's detailed results and remaining checks are in `.scratch/scripted-events/verification.md`.

To add an action later, derive from `ScriptedActionComponent`, read its exported fields in `InitializeCustomVariables`, implement validation/start and any timed update/cleanup, then register its custom type in `RegisterCustomComponents`. Actions update through the coordinator rather than advancing themselves independently.
