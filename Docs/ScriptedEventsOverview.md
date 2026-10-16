# How scripted events work

A scripted event is a list of instructions for a scene: move an object, play a sound, wait, hide something, or take control away from the player for a moment. Unreal is where you arrange the instructions. AGP runs them after the level is exported.

## For programmers

Each event lives on its own Actor. A `ScriptedEventComponent` is the coordinator, and the action components on that Actor are its instructions.

When AGP loads the exported level, the custom component registry creates the matching C++ components and their `InitializeCustomVariables` functions read the exported settings. During `BeginPlay`, the coordinator subscribes to its configured input action. That input calls `Play()`; other gameplay code can also call `Play()` directly.

Before starting, the coordinator checks all actions and their targets. It then runs actions in `Step` order. Actions with the same step start together. An action with `WaitForCompletion=true` holds that step open until it finishes. The World updates the coordinator every frame, and the coordinator advances the actions.

Targets are found by runtime Actor and component names. Movement uses another Actor as a destination marker. Audio uses a registered SoundID. Animation uses a clip alias or variable already available on the target animator. Exporting an action does not create the assets or animator it needs.

Completion or cancellation releases temporary control locks, movement overrides, and animation overrides. It does not rewind positions or undo changes to visibility, component enabled states, or animation variables. Started audio stops on cancellation or scene unload; non-waiting audio can continue after normal completion.

To add a new action, derive from `ScriptedActionComponent`, read its settings, implement its behavior and cleanup, and register its custom type. The old hard-coded demo is no longer required.

## For level designers using Unreal

Think of the event Actor as a recipe card. Its coordinator starts the recipe, and each action component describes one thing that should happen.

1. Create an **Actor Blueprint**, place an instance in your level, and add exactly one **ScriptedEventComponent**.
2. Add your action components to that same Blueprint. Give each component instance a unique name.
3. Set each action's **Step** number. Use different numbers for things that happen in order, or the same number for things that start together.
4. Fill in the target Actor names and the action's settings. Names must match the exported names exactly. Move actions also need a destination marker Actor; their time field is **DurationInSeconds**.
5. Set **WaitForCompletion** to true when the next step should wait for that action. For example, wait for a move to finish, but let a sound play while the next action begins.
6. Set the coordinator's **TriggerAction** to an existing AGP input action. For the current test binding, use `PlayUnrealSequenceTest`, which runs on **F2**. Set **Replayable=true** while testing.
7. Compile, save, export the level, and run it in **AGP**. Press F2 to test and F4 to reload the exported scene. Export again after making changes in Unreal.

For example:

| Step | Instructions |
| --- | --- |
| 0 | Lock player control. |
| 1 | Move an object to a marker and wait for it to arrive; start a sound without waiting for it. |
| 2 | Wait one second. |
| 3 | Unlock player control. |

Keep the event Actor active throughout playback. Actions should target separate Actors. A looping animation must not wait for completion; add a Wait action to keep the event running for the desired duration. Animation targets currently need their animator set up in AGP code.

For exact Actor types, component settings, and troubleshooting, follow the [Unreal test guide](ScriptedEventsUnrealTestGuide.md). The [component reference](ScriptedEvents.md) lists all available actions and fields.
