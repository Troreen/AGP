# Game loop and gameplay work

## Follow the code

Read these in order while stepping through a run:

1. [Main.cpp](../Source/Application/Game/Main.cpp) creates `Game` and `GameApplication`, then calls `application.Run(game)`.
2. [GameApplication.cpp](../Source/Application/Game/GameApplication.cpp) starts the window and services, loads scenes, and owns the frame loop.
3. [Game.cpp](../Source/Application/Game/Game.cpp) requests the initial scene and adds game-specific behavior.
4. [World.cpp](../Source/Engine/GameFramework/World/World.cpp) starts and updates components.
5. [WorldRenderer.cpp](../Source/Engine/GameFramework/Rendering/WorldRenderer.cpp) copies the live World into a render snapshot.

The useful breakpoints are `GameApplication::RunSession`, `Game::Initialize`, `GameApplication::ProcessPendingSceneLoad`, `World::BeginPlay`, and `GameApplication::RunMainLoop`.

## Starting and replacing a scene

1. `RunSession` creates the window, graphics, services, and input bindings.
2. `Game::Initialize` sets up game controls and calls `RequestSceneLoad` for the first scene.
3. `ProcessPendingSceneLoad` clears that request, chooses the exported file, and asks `UnrealSceneImporter` for `SceneData`.
4. `BuildWorldFromSceneData` creates a **candidate World**. `Game::ConfigureWorld` can add game-specific Actors and Components to it.
5. Once construction succeeds, `GameApplication` replaces the current World, selects a debug camera if the scene has no active camera, and calls `World::BeginPlay`.

Later scene requests follow the same path at the start of a frame. If import, construction, or `ConfigureWorld` fails, the current World keeps running. A successful scene change destroys the old World, so pointers to its Actors and Components must not be kept.

## One frame

`GameApplication::RunMainLoop` runs these steps in order:

1. Pump Windows messages. A quit request ends the loop.
2. Process a pending scene request before gameplay updates. The timer is reset after loading so load time is not counted as a gameplay frame.
3. Check the client area and render targets. If the client area is zero, skip this frame.
4. Measure `delta` and clamp it to at most 0.25 seconds. Update the input mapper so action listeners receive this frame's input.
5. Handle application controls such as the debug camera toggle.
6. Call `Game::Update(world, delta)` for game-level decisions.
7. Call `World::Update(delta)`. The World removes pending destroys, starts new Components, then updates enabled Components on active Actors.
8. Update audio, build a render snapshot from the World, and render it.

`Game::Update` runs **before** Component `Update` calls. An Actor or Component added during a World update begins play on the next World update because the current frame's Component list has already been collected.

## Where gameplay work goes

| Work | Place | ARPG example |
| --- | --- | --- |
| Bind a control used throughout the run | `Game::Initialize` | Bind attack, dodge, inventory, or pause actions. |
| Add behavior to each newly loaded scene | `Game::ConfigureWorld` | Attach health, combat, and enemy AI Components to imported Actors. |
| Make a game-wide decision each frame | `Game::Update` | Check whether the party has completed a dungeon objective. |
| Update one Actor's behavior | A `Component::Update` override | Count down an attack cooldown or advance an enemy state. |
| Start or stop a Component's subscriptions | `BeginPlay` / `EndPlay` | Listen for a player action and remove that listener when the player is removed. |
| Change maps | `GameApplication::RequestSceneLoad` | Enter a dungeon or return to town. |

Leave window messages, frame timing, World lifecycle calls, and rendering order in `GameApplication`. Gameplay code uses the World and Component APIs instead of calling `World::BeginPlay`, `World::Update`, or `World::Clear` itself.

## Use the system: player attack

Bind the control once in `Game::Initialize`:

```cpp
input.BindActionToInputCode("Attack", EKeyCode::MOUSELBUTTON);
```

Then attach a player combat Component in `Game::ConfigureWorld`. The scene must contain an Actor with the name you look up, or the game must spawn that Actor itself.

```cpp
if (Actor* player = world.FindActor("Player"))
{
    player->AddComponent<HealthComponent>("Health");
    player->AddComponent<CombatComponent>("Combat");
}
```

`HealthComponent` and `CombatComponent` are examples to implement, not existing engine classes. A `CombatComponent::Update(float delta)` might follow these steps:

```cpp
// CombatComponent::BeginPlay:
//   listen for "Attack" and save the listener ID.
//   when inputData.isPressed is true, set myAttackRequested.

// CombatComponent::Update:
// Reduce the attack cooldown by delta.
// If the player requested an attack and the cooldown is ready:
//   choose targets using a gameplay targeting system you add,
//   apply damage through their HealthComponent,
//   start the attack animation and sound,
//   reset the cooldown and clear the request.

// CombatComponent::EndPlay:
//   remove the input listener using the saved ID.
```

The input callback records intent; the Component owns the combat rule and advances it during the World update. The listener ID matters because a scene change destroys the old player. Remove the listener in `EndPlay` so later input cannot call back into a destroyed Component.

## Use the system: enemies, loot, and map changes

In `Game::ConfigureWorld`, find an imported enemy Actor by its authored name and attach the Components it needs. For example, a dungeon boss can receive `HealthComponent`, `EnemyAIComponent`, and `LootDropComponent`. Those are gameplay classes the team would add; the World does not provide combat or loot rules yet.

When an enemy dies, mark it with `actor->Destroy()`. Destruction is deferred until the World removes pending objects. If it should drop loot, spawn a new Actor with a unique name and attach its mesh and pickup Components. A Component added during the current World update begins play on the next update.

For a new map, add a `SceneId` value and its exported-file mapping in `GameApplication`. `Game::Initialize` receives the application; save a reference if later game logic needs to request a map change. When the player enters an exit portal, call `RequestSceneLoad(newSceneId)` through that reference. A later request is processed at the start of the next frame, before gameplay updates. Do not call `World::Clear` or replace `myWorld` from gameplay code.

## Lifecycle and ownership rules

- The World owns Actors; each Actor owns its Components. Pointers returned by `SpawnActor`, `FindActor`, `AddComponent`, and `GetComponent` are borrowed. Find them again after a scene replacement.
- `BeginPlay` runs when a Component joins a playing World. `Update` runs only while the Component is enabled and its Actor is active. `EndPlay` runs when a begun Component is removed or the World is cleared.
- A Component that registers an input listener in `BeginPlay` should remove that listener in `EndPlay`. Game-wide listeners registered in `Game::Initialize` are removed in `Game::Shutdown`.
- `Destroy()` stops an Actor or Component from taking part in later work; the World releases it during cleanup. Do not keep using its pointer after destruction or a scene change.
- Rendering needs an active camera. If the imported scene does not select one, `GameApplication` creates the debug camera before `BeginPlay`.

## Trace a problem

- **Input does nothing:** check the action binding in `Game::Initialize`, then the listener, then `InputMapper::Update` in the frame loop.
- **An Actor is missing:** check the exported name or the `SpawnActor` call, then `Game::ConfigureWorld` and the scene-load log.
- **A Component does not update:** check that it has begun play, is enabled, and belongs to an active Actor.
- **A scene change does not happen:** check `RequestSceneLoad`, `myPendingSceneId`, and `ProcessPendingSceneLoad`. A failed later load leaves the previous World running and writes a diagnostic.
- **A mesh is invisible:** check that the Actor and Component are active, that a camera is selected, and whether scene construction logged a missing mesh or material fallback.
