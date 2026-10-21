# First scripted event test: Unreal to AGP

This guide builds a sequence in Unreal that runs in AGP: a visible object moves to a marker, waits, disappears, reappears, and moves back. It tests export, component creation, field values, Actor lookup, step timing, replay, and scene reload. No AnimatorComponent is required.

The AGP instructions below follow the current repository code. Unreal menu labels can vary by editor version. The Unreal project and its exporter UI are not in this checkout, so use your project's existing export command when instructed to export.

## 1. Know which object types to use

An **Actor** is an object placed in a level. A **component** belongs to an Actor. A **Blueprint Class** is an asset you can reuse by dragging instances into a level.

Create these four level objects:

| Level Actor name | Unreal type | What it does |
| --- | --- | --- |
| `SE_WorkflowTest` | Instance of an Actor Blueprint, with **Actor** as its parent class | Holds one coordinator and all six actions. |
| `SE_TestObject` | **StaticMeshActor** | The visible object that moves and disappears. |
| `SE_Start` | **Target Point**, if your exporter includes it; otherwise a small StaticMeshActor | Stores the starting position. |
| `SE_End` | **Target Point**, if your exporter includes it; otherwise a small StaticMeshActor | Stores the destination position. |

The sequence Actor does not need a mesh. Keep these four Actors separate in the level; do not attach the moving object to the sequence Actor or a marker.

Use an existing test level whose floor, lighting, camera, and mesh export already work. That makes it easier to identify problems in the new sequence.

## 2. Create the sequence Actor Blueprint

1. Open the **Content Browser** or **Content Drawer** in Unreal.
2. Open a suitable folder for your test assets.
3. Right-click an empty area and choose **Blueprint Class**.
4. In the parent class picker, choose **Actor**.
5. Name the asset `BP_SE_WorkflowTest`.
6. Double-click it to open the Blueprint Editor.
7. Find the **Components** panel. Leave the default root component in place.

This is the standard Actor Blueprint creation workflow described in [Epic's Blueprint creation documentation](https://dev.epicgames.com/documentation/unreal-engine/creating-blueprint-classes-in-unreal-engine).

The actions will be added in the Components panel. You do not need to wire Event Graph nodes for this test: AGP's coordinator executes the exported action data.

## 3. Add the coordinator component

1. In the Blueprint's Components panel, click **Add** / **Add Component**.
2. Search for the scripted event component you already created in Unreal.
3. Add it and rename its component instance to `Sequence`.
4. Select `Sequence` and find its exported properties in the **Details** panel.
5. Set the following values:

| Field | Variable type | Value |
| --- | --- | --- |
| `TriggerAction` | String | `PlayUnrealSequenceTest` |
| `Replayable` | Bool | `true` |

If your components expose `StringVariables` and `BoolVariables` maps, add those keys to the matching maps. A map entry has a **key** (for example, `TriggerAction`) and a **value** (`PlayUnrealSequenceTest`). Use the map's plus button to add an entry. If your authoring setup exposes dedicated fields, fill those fields instead and confirm the export in step 8.

Keep exactly one ScriptedEventComponent on this Actor. Keep the sequence Actor active and its components enabled in AGP.

## 4. Create the visible object

1. Return to the Level Editor.
2. Find a static mesh asset that already exports and renders correctly in AGP.
3. Drag it from the Content Browser into the level. This normally creates a **StaticMeshActor**.
4. Select the new Actor in the **World Outliner**, press **F2**, and rename it `SE_TestObject`.
5. In Details, use **Transform > Location** to put it somewhere visible above the floor.
6. Keep its mesh component's local position at zero where practical, so the Actor origin is easy to use as a marker position.

A cube is fine if your pipeline already supports that mesh. A familiar project mesh is preferable to introducing an untested asset. Give the object no SpinComponent, AI, or other behavior that changes its transform. Keep Unreal physics simulation disabled for this test.

## 5. Create the position markers

### Option A: Target Point Actors

1. Open the **Place Actors** panel (usually under **Window > Place Actors** if hidden).
2. Search for **Target Point** and drag one into the level.
3. Rename it `SE_Start` in the Outliner.
4. Copy the Location values from `SE_TestObject` into `SE_Start`.
5. Add another Target Point named `SE_End`.
6. Give `SE_End` the same height and move it a few hundred units sideways across the floor. For example, add 300 to Unreal's X location.

The engine uses only the exported marker Actor's world position. It does not need an AGP-specific marker component. **Confirm that both Target Point Actors appear in your exported JSON.** Their support depends on your exporter.

### Option B: Small mesh markers

If Target Points are omitted from the export, drag two supported static mesh assets into the level, name them `SE_Start` and `SE_End`, and set their locations as above. Scale the marker meshes down so you can distinguish them from the moving object. Their visible meshes are just position indicators.

For either option, the moving object's Actor origin travels to the marker's Actor origin. Keep start and end at the same height for a horizontal move.

## 6. Add the six actions to the sequence Blueprint

Reopen `BP_SE_WorkflowTest`. Use **Add Component** to add each action component listed below. Two Move actions and two Wait actions are separate instances of their respective component types.

Rename each instance in the Components panel. Select each instance to edit its properties in Details.

| Instance name | Component type | `Step` (Int) | Other fields |
| --- | --- | --- | --- |
| `MoveOut` | `MoveActionComponent` | 0 | String `TargetActor=SE_TestObject`; String `DestinationActor=SE_End`; Float `DurationInSeconds=2.0` |
| `PauseAtEnd` | `WaitActionComponent` | 1 | Float `DurationInSeconds=1.0` |
| `HideObject` | `SetActorActiveActionComponent` | 2 | String `TargetActor=SE_TestObject`; Bool `Active=false` |
| `PauseHidden` | `WaitActionComponent` | 3 | Float `DurationInSeconds=1.0` |
| `ShowObject` | `SetActorActiveActionComponent` | 4 | String `TargetActor=SE_TestObject`; Bool `Active=true` |
| `MoveBack` | `MoveActionComponent` | 5 | String `TargetActor=SE_TestObject`; String `DestinationActor=SE_Start`; Float `DurationInSeconds=2.0` |

For **every action**, also set Bool `WaitForCompletion=true` and leave String `TargetComponent` empty. Leave `TargetActor` empty on the two Wait components.

When using variable maps:

- `IntVariables`: `Step`.
- `FloatVariables`: `DurationInSeconds`.
- `StringVariables`: `TargetActor`, `DestinationActor`, and `TargetComponent` where applicable.
- `BoolVariables`: `WaitForCompletion` and `Active` where applicable.

Use **DurationInSeconds**, including the spelling and capitalization. This is the field read by the current implementation.

Steps run from lowest to highest. Actions sharing a step start together. These distinct steps make the first test easy to follow. The Move action interpolates position directly; it does not navigate around obstacles or turn the object.

Click **Compile**, then **Save** in the Blueprint Editor.

## 7. Place the sequence in the level

1. Drag `BP_SE_WorkflowTest` from the Content Browser into the level.
2. Rename this placed instance to `SE_WorkflowTest` in the World Outliner.
3. Put it anywhere convenient. Its own position does not affect this sequence.
4. Save the level.

Creating the Blueprint asset alone is insufficient: an instance must be placed in the exported level.

The intended ownership is:

```text
SE_WorkflowTest (Actor Blueprint instance)
  Default root component
  Sequence       (ScriptedEventComponent)
  MoveOut        (MoveActionComponent)
  PauseAtEnd     (WaitActionComponent)
  HideObject     (SetActorActiveActionComponent)
  PauseHidden    (WaitActionComponent)
  ShowObject     (SetActorActiveActionComponent)
  MoveBack       (MoveActionComponent)

SE_TestObject    (separate StaticMeshActor)
SE_Start         (separate marker Actor)
SE_End           (separate marker Actor)
```

## 8. Export and inspect the JSON

Use your project's existing Unreal level exporter. Export the scene and any mesh/material assets required by your normal workflow.

Open the resulting level JSON in a text editor and check:

1. The `Actors` array contains all four named Actors.
2. The sequence Actor's `Components` array contains its coordinator and all six actions.
3. The exported Actor `Name` values match the strings used in the actions. Outliner labels and exported names are not guaranteed to match in every exporter.
4. Every component instance has a unique `Name` within the sequence Actor.
5. Each component has the expected `CustomType` and flattened `Custom` values.

For example, part of the MoveOut record should be:

```json
{
  "Name": "MoveOut",
  "CustomType": "MoveActionComponent",
  "Custom": {
    "Step": 0,
    "WaitForCompletion": true,
    "TargetActor": "SE_TestObject",
    "TargetComponent": "",
    "DestinationActor": "SE_End",
    "DurationInSeconds": 2.0
  }
}
```

This is a partial record for comparison; keep the exporter's other required fields such as `TypeID`, `Parent`, `Tags`, and `Transform`.

The coordinator's custom data should contain:

```json
"CustomType": "ScriptedEventComponent",
"Custom": {
  "TriggerAction": "PlayUnrealSequenceTest",
  "Replayable": true
}
```

The current custom component registry removes a trailing `_C`, so `MoveActionComponent_C` also resolves. It does not remove a `BP_` prefix: `BP_MoveActionComponent_C` would require a matching registration or an exporter type override. The class string in the export is what matters.

Do not nest `StringVariables` or the other maps inside the exported `Custom` object. The exporter should flatten their entries. Floats must export as float values such as `2.0`; integers such as `Step` must remain integers such as `0`. The importer preserves these types.

## 9. Give the sequence its own AGP key binding

The current input settings bind **F2** to PlayUnrealSequenceTest. The built-in C++ demo has been removed. Keep this binding for the exported sequence; if it is missing in another checkout, add it as follows.

1. Open [InputBindings.json](../Bin/Settings/InputBindings.json).
2. Add the entry below inside both the `defaults` and `current` objects.
3. Add a comma after the preceding entry. JSON does not allow a trailing comma after the last entry.
4. Save the file and restart AGP after changing it.

```json
"PlayUnrealSequenceTest": {
  "key": "F2",
  "pointer": null,
  "gamepad": null
}
```

`TriggerAction` must match `PlayUnrealSequenceTest` exactly. The coordinator listens for an existing AGP input action; it does not create a binding or use Unreal input settings.

## 10. Load the exported level in AGP

1. Place the export under the normal `Content/ExportedScenes` directory, preserving any required accompanying files and folders.
2. Open [ApplicationSettings.json](../Bin/Settings/ApplicationSettings.json).
3. Under `current.application`, set `initialScene` to your level JSON path relative to `Content/ExportedScenes`.
4. For the existing component test export, that value is `lvl_component_test/Lvl_Component_Test_Level.json`.
5. Build `Game.sln` in Visual Studio using **Debug | x64** if your executable does not include the current scripted event code.
6. Run `Bin/Debug/Game.exe`.

Use the existing camera setup in your test level to view the object and both markers. F1 toggles the debug camera if needed; hold the right mouse button and use WASD to move that camera.

Unreal's Play button is not the playback check for AGP's C++ implementation. Unreal authors the data; run the exported scene in AGP to exercise the system.

## 11. Run the checks

| Action | Expected result |
| --- | --- |
| Press F2 | Object moves to SE_End over two seconds, waits one second, disappears for one second, reappears, and returns over two seconds. |
| Press F2 after completion | The whole sequence repeats. |
| Press F2 repeatedly while moving | The running sequence continues without restarting. |
| Press F4 during movement | The current scene reloads and the old sequence stops. |
| Press F2 after the reload | The newly loaded sequence runs again. |

Small frame delays between steps are normal. The whole sequence takes approximately six seconds.

To prove an Unreal edit reaches the engine, change MoveOut's `DurationInSeconds` from `2.0` to `4.0` in Unreal, compile/save, export again, then reload the scene with F4. The outward move should now take four seconds. Changing only the Unreal asset without exporting will not update AGP.

## 12. If it does not work

| Symptom | What to check |
| --- | --- |
| Nothing happens on F2 | The correct level is loaded; the sequence Actor exists in the export; the input entry is in `current`; TriggerAction matches; AGP was restarted after the binding change. |
| A custom component is unknown | Compare exported CustomType with the registered types in `RegisterCustomComponents.cpp`; check prefixes and spelling. |
| Console reports a missing target or destination | Compare strings with exported Actor Name values; check that marker Actors survived export. |
| Movement takes one second instead of two | Check `DurationInSeconds` spelling and float type. Missing duration fields use the default of one second. |
| A later action is invalid and nothing starts | The coordinator validates the entire sequence before starting any action. Fix the later action as well. |
| The object starts in an unexpected place | Check Actor positions, mesh local offsets, and other components that change the transform. |

Playback validation errors are printed to the debug console with a prefix like `Sequence 'SE_WorkflowTest': ...`.

## 13. Test audio, component enabling, and player control

Do this after the first sequence works. Keep the existing sequence Actor and F2 binding, and replace its six-action schedule with the expanded schedule below. Update the existing actions rather than leaving duplicate moves attached.

### Add a separate object for the component enable test

1. Create another **Blueprint Class** with **Actor** as its parent, named `BP_SE_Spinner`.
2. Open it and add a **Static Mesh** component in the Components panel.
3. Select that component, then assign a supported mesh in its Details panel's **Static Mesh** asset field. Use a rectangular or asymmetric object so rotation is visible.
4. Add your existing exported **SpinComponent**, following the same authoring pattern as the working Spin example.
5. Rename the Spin component instance to `TestSpin`.
6. Set Float `SpinSpeed=90.0` and String `TargetComponentName` to empty. This spins the whole Actor.
7. Compile and save. Place an instance in the level, name it `SE_Spinner`, and position it near the test object where the camera can see both.
8. Export and confirm that its component record is named `TestSpin`, with `CustomType=SpinComponent` or `SpinComponent_C`.

Do not add Spin to the moving test object. The separate spinner makes the enable/disable result clear.

### Replace the action schedule

All rows are components on `SE_WorkflowTest`. Keep TargetComponent empty unless the table says otherwise. Set WaitForCompletion=true unless explicitly false. Each row needs its own uniquely named component instance.

| Step | Instance name | Component type | Fields |
| --- | --- | --- | --- |
| 0 | `LockPlayer` | LockControlActionComponent | TargetActor=`Player` |
| 1 | `PauseSpin` | SetComponentEnabledActionComponent | TargetActor=`SE_Spinner`; TargetComponent=`TestSpin`; Enabled=false |
| 2 | `AudioBeforeMove` | PlayAudioActionComponent | Sound=5; Positional=false; WaitForCompletion=true; TargetActor empty |
| 3 | `MoveOut` | MoveActionComponent | TargetActor=`SE_TestObject`; DestinationActor=`SE_End`; DurationInSeconds=2.0 |
| 3 | `RoarAtObject` | PlayAudioActionComponent | TargetActor=`SE_TestObject`; Sound=7; Positional=true; WaitForCompletion=false |
| 4 | `PauseAtEnd` | WaitActionComponent | DurationInSeconds=1.0 |
| 5 | `HideObject` | SetActorActiveActionComponent | TargetActor=`SE_TestObject`; Active=false |
| 6 | `PauseHidden` | WaitActionComponent | DurationInSeconds=1.0 |
| 7 | `ShowObject` | SetActorActiveActionComponent | TargetActor=`SE_TestObject`; Active=true |
| 8 | `MoveBack` | MoveActionComponent | TargetActor=`SE_TestObject`; DestinationActor=`SE_Start`; DurationInSeconds=2.0 |
| 9 | `ResumeSpin` | SetComponentEnabledActionComponent | TargetActor=`SE_Spinner`; TargetComponent=`TestSpin`; Enabled=true |
| 10 | `UnlockPlayer` | UnlockControlActionComponent | TargetActor=`Player` |
| 11 | `ObserveHandoff` | WaitActionComponent | DurationInSeconds=3.0 |

Use IntVariables for Step and Sound; FloatVariables for DurationInSeconds; BoolVariables for Enabled, Active, Positional, and WaitForCompletion; and StringVariables for names.

The current Game.cpp creates `Player` and its PlayerController in C++. Do not place a second Actor named Player in Unreal. Locking applies to that runtime player, while the mesh remains the movement target. Test with the gameplay camera and commands that work before playback; debug camera movement is not the player control lock test. If gameplay movement is unavailable in your scene, inspect PlayerController::IsScriptedControlLocked() in the debugger to verify the lock and unlock.

### Audio IDs and expected behavior

The current [SoundID enum](../Source/Engine/GameFramework/AudioManager.h) assigns **5 to eCollectItem** and **7 to eHeavyRoar**. Put `Sound` in `IntVariables`. Both are registered by AudioManager. These are integer IDs, not Unreal sound assets or FMOD event paths. Recheck the enum if someone changes its ordering.

The expected run is:

1. The spinner rotates before F2 is pressed.
2. F2 locks player control and stops the spinner without hiding it.
3. Collect Item plays. Movement waits for FMOD to report that playback has ended.
4. The object moves out while Heavy Roar starts at its position. The movement does not wait for the roar to finish.
5. The original wait/hide/show/return sequence runs.
6. The spinner resumes, then player control is explicitly unlocked.
7. During the last three-second wait, player control already works even though the sequence is still running. This checks the Unlock action rather than relying only on completion cleanup.

Ensure the normal build has copied the FMOD banks beside Game.exe under Audio, and that master/SFX volume is audible. Audio waiting uses the FMOD playback state, so compare the start of movement with the sound ending. Do not use looping music for a waiting audio action.

Positional=true supplies a world position to FMOD when the action starts. The sound stays at that captured position; it does not follow the object. Audible distance or panning also depends on the FMOD event's spatial settings and the listener, so a stereo impression alone does not prove spatial behavior. Inspect the supplied location in AudioManager::PlayScriptedAudio if the bank does not make the difference audible.

The listener follows the active camera each frame, including the debug camera. The current audio conversion is 1,000 engine units per FMOD unit. The shipped Heavy Roar event reports a minimum distance of 1 and maximum of 20 FMOD units, corresponding to 1,000 and 20,000 engine units. Compare the roar after moving the camera several thousand units away and replaying with F2; a short move may produce little difference. Collect Item in this sequence has Positional=false and is expected to stay the same loudness as you move. Offline metering confirmed the roar attenuates with distance, but the in-game listening check must still be performed after rebuilding.

Repeat the sequence and reload with F4 during a run. Started scripted sounds should stop on scene teardown. Non-waiting audio is allowed to continue after normal sequence completion. Each SoundID shares one registered playback; avoid triggering another event using the same sound IDs during this test. The game's background Castle Ambience uses a different ID; leave that ID out of these test actions.

Activation and component enabled states persist after cancellation. Reload with F4 to restore the scene if you interrupt after HideObject or PauseSpin; cancellation does not undo those authored changes. Temporary player control locks are released by sequence cleanup.

## 14. Test both animation actions without animator import

The remaining action types are **PlayAnimationActionComponent** and **SetAnimationVariableActionComponent**. Their actions can be authored/exported from Unreal now, but their target needs a working runtime AnimatorComponent. An ordinary StaticMeshActor cannot supply one.

For direct playback alone, the existing C++ Actor `Enemy` already has a registered `Idle` clip. It has no animation tree, so it cannot test animation variables. Its enemy behavior also makes it less convenient as a stationary test fixture.

For a visible test of direct playback, bools, and triggers, add the following optional fixture in `Game::ConfigureWorld` in [Game.cpp](../Source/Application/Game/Game.cpp), before the `Temporary place for debug animation` block that can return early. The required includes are already present there. This is a suggested manual code addition; this guide does not apply it for you.

```cpp
{
    Actor* actor = aWorld.SpawnActor("SE_AnimationTarget");
    actor->GetTransform().SetWorldPosition({0.0f, 0.0f, 0.0f});

    AssetRegistry& assets = ServiceLocator::GetInstance().GetAssetRegistry();
    auto* mesh = actor->AddComponent<SkeletalMeshComponent>("TestMesh");
    mesh->SetMesh(assets.GetAsset<MeshAsset>("SK_C_Tga_Bro.fbx"));

    auto* animator = actor->AddComponent<AnimatorComponent>("TestAnimator");
    animator->SetMeshComponent(mesh);
    animator->SetTree("BroTree");
    animator->GetTree().SetBool("IsIdle", true);
    animator->GetTree().SetBool("IsWalking", false);
    animator->GetTree().SetBool("IsRunning", false);
}
```

Place the fixture at a visible, clear **engine world position** appropriate for your scene; the origin above is just a starting value. Confirm that the Bro mesh and the BroTree clips exist in your Content checkout and load successfully. These asset/tree names match the existing sample setup in Game.cpp and Content/Animations/Animation Manager. Rebuild the game after adding the fixture. Do not also export an Actor named SE_AnimationTarget.

### Author a second event for animation

Create a new Actor Blueprint named `BP_SE_AnimationTest`, place an instance named `SE_AnimationTest`, and add exactly one ScriptedEventComponent. Set TriggerAction=`PlayAnimationSequenceTest` and Replayable=true. Add the following actions to this new sequence Actor.

For all animation actions, set TargetActor=`SE_AnimationTarget`, TargetComponent=`TestAnimator`, and WaitForCompletion=true unless specified otherwise. Wait actions need neither target field.

| Step | Instance name | Component type | Fields |
| --- | --- | --- | --- |
| 0 | `StartWalking` | SetAnimationVariableActionComponent | Variable=`IsWalking`; Trigger=false; Value=true |
| 1 | `ObserveWalk` | WaitActionComponent | DurationInSeconds=2.0 |
| 2 | `StopWalking` | SetAnimationVariableActionComponent | Variable=`IsWalking`; Trigger=false; Value=false |
| 3 | `ObserveIdle` | WaitActionComponent | DurationInSeconds=1.0 |
| 4 | `WaveTrigger` | SetAnimationVariableActionComponent | Variable=`IsWaving`; Trigger=true; Value=true (ignored for triggers) |
| 5 | `ObserveWave` | WaitActionComponent | DurationInSeconds=5.0 |
| 6 | `DirectWalk` | PlayAnimationActionComponent | Animation=`Walk`; Loop=false; WaitForCompletion=true |
| 7 | `DirectIdleLoop` | PlayAnimationActionComponent | Animation=`Idle`; Loop=true; WaitForCompletion=false |
| 7 | `HoldLoop` | WaitActionComponent | DurationInSeconds=2.0 |

Add a `PlayAnimationSequenceTest` entry bound to **F9** in both defaults and current in InputBindings.json, using the same format as the F2 entry. Restart after changing the binding, export the new sequence, and load that scene.

BroTree defines the bool IsWalking and trigger IsWaving. Its registered clip aliases are state names such as Walk and Idle, not asset filenames. This fixture has no movement behavior, so walking animates in place. Expect walk, idle, wave, one non-looping direct Walk, then a two-second direct Idle loop. After the sequence, the tree regains animation control. The five-second wait is an observation window, not a guarantee that a changed wave asset has finished; extend it if necessary.

Setting an animation variable completes immediately. It does not wait for the resulting animation. A direct non-looping clip can wait for playback; a looping direct clip must use WaitForCompletion=false and another waiting action to hold the event open. The tree's current clip restarts when the direct override releases; it does not resume a saved playback time.

Check both replay and F4 during direct playback. Use a breakpoint in SetAnimationVariableActionComponent::Start if a variable change needs confirmation. The runtime target is created in C++, so this tests Unreal-authored animation actions reaching the runtime animator; it does not establish animator or skeletal asset import from Unreal.

## 15. Where the event runs, and whether the demo is required

The exported event runs inside **AGP's Game.exe**. The path is:

```text
Unreal level and component data
  -> exported scene JSON
  -> UnrealSceneImporter produces SceneData
  -> SceneWorldBuilder creates Actors and components
  -> CustomComponentRegistry creates registered custom component types
  -> InitializeCustomVariables reads each component's exported fields
  -> Game::ConfigureWorld adds any C++ scene fixtures
  -> World::BeginPlay calls ScriptedEventComponent::BeginPlay
  -> coordinator subscribes to its TriggerAction
  -> F2 input calls that coordinator's Play()
  -> World::Update calls ScriptedEventComponent::Update each frame
  -> coordinator starts, ticks, and cleans up its actions
```

The import/build/scene lifecycle is driven by [GameApplication.cpp](../Source/Application/Game/Runtime/GameApplication.cpp). The event logic lives in [ScriptedEventComponent.cpp](../Source/Application/Game/Components/ScriptedEvents/ScriptedEventComponent.cpp), and action implementations live in [ScriptedActionComponent.cpp](../Source/Application/Game/Components/ScriptedEvents/ScriptedActionComponent.cpp). The coordinator advances the actions; they do not independently advance the sequence through their own normal component Update calls.

The built-in C++ demo and its hookup have been removed. Exported sequences run through their own coordinators. F2 triggers the exported test event.

The runtime still requires ScriptedEventComponent, ScriptedActionComponent and their derived action types, their registrations in RegisterCustomComponents.cpp, and the normal scene loading/world update path. Game.cpp still creates Player for the optional control test; the optional animation fixture above is separate from the removed demo.

## 16. Action coverage checklist

These are manual checks to perform, not claims that the editor round trip has already passed.

| Action type | Check in this guide |
| --- | --- |
| WaitActionComponent | Visible pauses and held observation windows. |
| MoveActionComponent | Outward/return motion and changed duration after re-export. |
| SetActorActiveActionComponent | Whole object disappears and reappears. |
| SetComponentEnabledActionComponent | Spinner stops and resumes while remaining visible. |
| PlayAudioActionComponent | Waiting audio, non-waiting audio, positional source, replay, scene teardown. |
| LockControlActionComponent | Player commands blocked during the expanded mesh sequence. |
| UnlockControlActionComponent | Player commands restored during the final observation wait. |
| PlayAnimationActionComponent | Non-looping completion, held loop, return to animation tree. |
| SetAnimationVariableActionComponent | Bool changes walk/idle; trigger starts wave. |

This guide was prepared from the current source and settings. The actual Unreal export and visual playback still need to be performed with your editor project.
