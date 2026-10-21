# Set up a scripted event

A scripted event is an Actor with one `ScriptedEventComponent` and several action components. Each action does one job: move, wait, animate, play a sound, or change another Actor.

## 1. Create the event in Unreal

1. Create an Actor Blueprint
2. Open the Blueprint and add only one `ScriptedEventComponent`.
3. Add the action components you need. Give every component a unique name.
4. Place the Blueprint in the level. Give target Actors and destination markers unique names too.

In each component's Details, put fields in the matching map. Spelling and capitalization must match:

| Map | Fields |
| --- | --- |
| `StringVariables` | Names such as `TargetActor`, `DestinationActor`, `Variable` |
| `IntVariables` | `Step`, `Sound` |
| `FloatVariables` | `DurationInSeconds` |
| `BoolVariables` | `WaitForCompletion`, `Value`, `Enabled`, etc. |

## 2. Choose how it starts

Set these fields on `ScriptedEventComponent`:

| Field | Type | Use |
| --- | --- | --- |
| `TriggerAction` | String | Use for debugging via input buttons, such as `F2` needs Engine side setup. Leave blank for collision-triggered events. |
| `TriggerCollider` | String | Optional name of a Box or Sphere component on the event Actor, such as `TriggerBox`. Player entry starts the event and disables that collider. Leave blank for manual playback. |
| `Replayable` | Bool | `true` allows repeated playback after the event stops. Default is `false`. A consumed collision trigger stays disabled, even with replay enabled. So technically this feature is non-functional. |

Keep the event Actor active and its coordinator enabled. Collision triggering also needs the player's collider to accept the Trigger layer. Currently we have a debug collider on the player that is not Unreal authored.

## 3. Set action order and targets

Every action uses these common fields:

| Field | Type | Use |
| --- | --- | --- |
| `Step` | Int | Starts at `0`. Lower steps run first; actions with the same step start together. |
| `WaitForCompletion` | Bool | Default `true`: hold this step until the action finishes. `false`: let later steps proceed while it continues. |
| `TargetActor` | String | Exact runtime Actor name that the action will affect, for example `Player` or `Enemy_1`. Leave empty on waits and non-positional audio. |
| `TargetComponent` | String | Component instance name. Usually leave empty to select the target's animator/controller automatically. Required when enabling or disabling a component. For example `EnemyBrainComponent` |

Targets must be separate from the event Actor. The next step starts when all waiting actions finish. Put actions that depend on one another in different steps. Two events cannot control the same target Actor at once.

## 4. Pick your actions

| Action component | How to use it |
| --- | --- |
| `WaitActionComponent` | Set float `DurationInSeconds`, for example `1.0`, to pause the sequence. No target needed. |
| `MoveActionComponent` | Set `TargetActor`, string `DestinationActor` to a marker's name, and float `DurationInSeconds`. It moves straight to the marker's starting position. Bool `FaceMovementDirection` defaults to `true`; set `false` to keep facing. Movement does not choose an animation or avoid obstacles. |
| `SetAnimationVariableActionComponent` | Change a setting in the target's animation tree. For example, set the player's `IsRunning` to `true` to use running, or `false` to use idle. See the field examples below. |
| `PlayAnimationActionComponent` | Set `TargetActor` and string `Animation` to a registered clip alias. Bool `Loop=false` plays once. It temporarily overrides the animation tree; finishing returns control to the tree and restarts its current clip. Looping requires `WaitForCompletion=false` and another waiting action to keep the event running. Avoid overlapping direct clips on the same Actor. |
| `PlayAudioActionComponent` | Start a registered sound using int `Sound` (see sound IDs below). Set `WaitForCompletion=false` to keep playing it while later steps run, or `true` to hold this step until the sound ends. See the examples below. |
| `LockControlActionComponent` | Set `TargetActor=Player` to stop current movement and block player commands. Scripted movement still works. Control is released when the event ends. |
| `UnlockControlActionComponent` | Set `TargetActor=Player` to release this event's control lock at a chosen step. |
| `SetActorActiveActionComponent` | Set `TargetActor` and bool `Active`. `false` hides the Actor and stops its updates; `true` activates it again. The result stays after the event. |
| `SetComponentEnabledActionComponent` | Set `TargetActor`, `TargetComponent`, and bool `Enabled`. For example, disable `EnemyBrainComponent` without hiding its mesh. The result stays after the event. |

Durations must be nonnegative. Keep animated Actors active and their animators enabled. The event ends after its final waiting action; unfinished scripted movement and direct animation are released then.

### SetAnimationVariable: running and idle

This action changes a named variable in an Actor's existing animation tree. The tree uses that variable to decide which animation to play. The action does not select a clip directly or move the Actor.

For the player's current tree, use these settings:

| Field | Start running | Return to idle | Type/map |
| --- | --- | --- | --- |
| `TargetActor` | `Player` | `Player` | String |
| `Variable` | `IsRunning` | `IsRunning` | String |
| `Trigger` | `false` | `false` | Bool |
| `Value` | `true` | `false` | Bool |

Keep `Trigger=false` for these bool settings. Set `Value=true` alongside the Move action, then set `Value=false` in the step after the move finishes. The current player tree returns to Idle when `IsRunning=false`; you do not need to set a separate `IsIdle` variable. Movement does not change animation variables automatically.

**This action finishes after setting the variable.** `WaitForCompletion=true` does not wait for the resulting animation to finish. Use the Move action's completion to control when running ends, or a Wait action to hold an idle pause.

### PlayAnimation: play a scripted clip once

For a scripted roar, gesture, or other one-off animation, use `PlayAnimationActionComponent`:

| Field | Example | Type/map |
| --- | --- | --- |
| `TargetActor` | The Actor that should roar | String |
| `Animation` | `Roar` (must already be registered on its animator) | String |
| `Loop` | `false` | Bool |
| `WaitForCompletion` | `true` | Bool |

This plays the clip directly and holds the step until it finishes. The animation tree then takes control again, restarting its current state's clip. No roar variable or roar transitions are needed in the tree. `Roar` here is an example alias to register, not an existing player animation. Without an animation tree, the Actor holds the final pose instead.

Set `WaitForCompletion=false` if later steps should run while the clip plays. Keep the event running long enough for the clip to finish. Playing a clip does not stop movement or AI; lock control or disable the relevant behavior separately when needed.

**Optional: animation-tree triggers.** Use `SetAnimationVariableActionComponent` with `Trigger=true` only when an existing trigger variable and the tree's transition rules should control playback. The variable must be declared as `Trigger` in [`AnimationManager.json`](<../Content/Animations/Animation Manager/AnimationManager.json>); `Value` is ignored, and the tree clears the request when it uses it. For a simple scripted one-off clip, use PlayAnimation as above.

### PlayAudio: choose whether the sequence waits

**You can start a sound and do other actions while it plays.** Set `WaitForCompletion=false` on the audio action. This starts the sound and lets the sequence move to its next step without waiting for the sound to end. Other waiting actions in the same step still need to finish.

| Audio setting | What happens |
| --- | --- |
| `WaitForCompletion=false` | The sound plays while later steps run. Use for background dialogue, music, or a sound over movement. |
| `WaitForCompletion=true` (default) | The next step waits until the sound stops. Use when something should happen after a line of dialogue or sound finishes. Do not use for looping sounds. |

For example, to play dialogue while the player moves:

| Step | Action | Settings |
| --- | --- | --- |
| 0 | PlayAudio | `Sound=<registered dialogue ID>`, `WaitForCompletion=false` |
| 1 | Move | `TargetActor=Player`, `DestinationActor=SE_End`, `DurationInSeconds=3.0`, `WaitForCompletion=true` |
| 2 | Hide a prop | Runs after the 3-second move, even if the dialogue is still playing. |

If you change that audio action to `WaitForCompletion=true`, the move starts **after the dialogue ends**. To start sound and movement together but wait for both before the next step, give both actions the same `Step` and set both to `WaitForCompletion=true`.

For audio location, set bool `Positional=false` when no Actor location is needed. Set `Positional=true` and `TargetActor` to play at that Actor's position when the sound starts; the sound does not follow later movement.

Non-waiting audio can keep playing after normal sequence completion. Cancelling the event or unloading the scene stops sounds started by it.

## Extra details: when is an action complete?

Each action reports completion through `Tick()`: `true` means finished; `false` means still running. The sequence checks every frame and advances when **all actions in the current step with `WaitForCompletion=true` have finished**. Setting it to `false` does not make an action finish sooner; it only stops that action from holding up the next step.

| Action | Completed when... |
| --- | --- |
| Wait | `DurationInSeconds` has elapsed. |
| Move | Its duration has elapsed and the Actor has been placed at the destination. |
| PlayAnimation | The non-looping clip finishes playing. |
| PlayAudio | FMOD reports that the sound has stopped. |
| SetAnimationVariable | The variable is set, **not** when the resulting animation finishes. |
| LockControl / UnlockControl | The control change is applied. The lock itself can remain until a later unlock or event cleanup. |
| SetActorActive / SetComponentEnabled | The setting is applied. |

One-time changes finish on the first sequence update after starting. Looping clips and sounds do not finish naturally, so they must not hold a step open.

For example:

| Step | Action | `WaitForCompletion` |
| --- | --- | --- |
| 0 | Move the player for 3 seconds | `true` |
| 0 | Play a 5-second sound | `false` |
| 1 | Hide a prop | `true` |

Both step 0 actions start together. After **3 seconds**, movement finishes and step 1 hides the prop while the sound continues. If the sound also waits, step 1 starts after **5 seconds**. If neither action waits, step 1 starts on the next sequence update.

Keep a waiting action alive for as long as movement or direct animation is needed: ending the whole event releases those actions, even if they have not finished. Non-waiting audio can continue after normal event completion.

## Where to find sound names and animation names

**Sounds:** Put `Sound` in `IntVariables` using a numeric ID from [`AudioManager.h`](../Source/Engine/GameFramework/AudioManager.h): for example, 5 for eCollectItem, 7 for eHeavyRoar, or 12 for eSlash. IDs must be between 0 and eSoundCount - 1. Recheck IDs if the enum ordering changes. Avoid overlapping actions using the same sound: they share one playback.

The current registered sounds, including music and ambience, are listed below. Event paths come from [`AudioManager::RegisterAllEvents`](../Source/Engine/GameFramework/AudioManager.cpp).

| `Sound` integer | Sound ID | FMOD event |
| --- | --- | --- |
| 0 | `eMainTheme` | `event:/FMODTest` |
| 1 | `eBlizzardAmbience` | `event:/Blizzard Ambience` |
| 2 | `eCastleAmbienceOne` | `event:/Castle Ambience 1` |
| 3 | `eCastleAmbienceTwo` | `event:/Castle Ambience 2` |
| 4 | `eCombatOne` | `event:/Combat One` |
| 5 | `eCollectItem` | `event:/Collect Item` |
| 6 | `eCrossbowCock` | `event:/Crossbow Cock` |
| 7 | `eHeavyRoar` | `event:/Heavy Roar` |
| 8 | `eIntroSwell` | `event:/Intro Swell` |
| 9 | `ePotion` | `event:/Potion Drink` |
| 10 | `eSickleSlash` | `event:/Sickle Slash` |
| 11 | `eSlashHit` | `event:/Slash Hit` |
| 12 | `eSlash` | `event:/Slash` |

`eSoundCount` is 13 and is a count, not a playable sound ID.

**Animation variables:** Find the target's tree in [`AnimationManager.json`](<../Content/Animations/Animation Manager/AnimationManager.json>). Its `Variables` list gives names and whether each is a bool or trigger.

**Clip aliases:** Check the tree files in [`Trees`](<../Content/Animations/Animation Manager/Trees>), or the Actor's C++ `AddAnimation` calls. Use the registered alias, not an FBX path. For `PlayerTree`, the aliases are `Idle` and `Running`; setting `IsRunning=true` selects running, and `false` selects idle.

## Example: run, pause, play a slash sound

Add these actions to one event Actor. All use `WaitForCompletion=true`:

| Step | Action | Settings |
| --- | --- | --- |
| 0 | LockControl | `TargetActor=Player` |
| 1 | SetAnimationVariable | `TargetActor=Player`, `Variable=IsRunning`, `Trigger=false`, `Value=true` |
| 1 | Move | `TargetActor=Player`, `DestinationActor=SE_End`, `DurationInSeconds=3.0`, `FaceMovementDirection=true` |
| 2 | SetAnimationVariable | `TargetActor=Player`, `Variable=IsRunning`, `Trigger=false`, `Value=false` |
| 2 | Wait | `DurationInSeconds=1.0` |
| 3 | PlayAudio | `Sound=12`, `Positional=true`, `TargetActor=Player` |
| 4 | UnlockControl | `TargetActor=Player` |

Compile and save the Blueprint and level, then **export the level using the TGA exporter** into AGP's `Content/ExportedScenes`. Load that scene in AGP and test!
