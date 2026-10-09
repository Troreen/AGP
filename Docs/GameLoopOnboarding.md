## Start in Main.cpp

[Main.cpp](../Source/Application/Game/Main.cpp) loads settings and initializes the asset registry, animation manager, and audio. Near the bottom, it creates `Game` and `GameApplication` and calls `application.Run(game)`.

`GameApplication` runs the application: the window, scene loading, frame timing, and rendering. `Game` is where our game code starts. Most gameplay will live in Components attached to Actors.

Open [GameApplication.cpp](../Source/Application/Game/GameApplication.cpp) and find `RunSession`. After the window, services, and input are ready, it calls `InitializeGameSession`, then `RunMainLoop`.

## How a level becomes a World

In `InitializeGameSession`, we first call `Game::Initialize`. Right now, that registers the scene-reload listener and plays the intro sound. It can also request a starting scene. If it doesn't, the application uses `initialScene` from [ApplicationSettings.json](../Bin/Settings/ApplicationSettings.json).

Next, open `ProcessPendingSceneLoad`. Follow these calls:

1. Take the pending scene path and clear the request.
2. Find the file under `contentRoot / SceneFolder / requestedScene`.
3. Read it with `UnrealSceneImporter`, which returns `SceneData`.
4. Use `BuildWorldFromSceneData` to create a new World with Actors and Components.
5. Call `Game::ConfigureWorld` on that new World.
6. Clear the old World and replace it with the new one.
7. Add and select a debug camera if there isn't an active camera, then call `World::BeginPlay`.

The importer reads data; the builder turns that data into objects. Open [SceneData.h](../Source/Engine/GameFramework/Scenes/SceneData.h) to see the records, then [WorldFromSceneData.cpp](../Source/Engine/GameFramework/Scenes/WorldFromSceneData.cpp) to see Actor spawning and `CreateComponent`. A mesh record becomes a mesh Component, a light record becomes a light Component, and so on.

We build the new World before replacing the old one. For later loads, ordinary import or construction errors are logged and leave the old World running. That recovery also covers `ConfigureWorld`. It doesn't cover allocation failures, or camera setup and `BeginPlay` after the replacement.

## What happens each frame

Back in `GameApplication::RunMainLoop`, the order is:

1. Handle Windows messages. Stop if we're quitting, and update settings if the window changed monitors.
2. Load a pending scene, if there is one. Reset the timer afterward so loading time doesn't become gameplay time.
3. Check the window size and resize render targets if needed. Skip updates and rendering while the client area is zero.
4. Measure `delta`, clamped between 0 and 0.25 seconds. An invalid value becomes zero.
5. Update input. Listeners run here; if one requests a quit, stop the loop.
6. Handle the debug camera toggle.
7. Call `Game::Update`.
8. Call `World::Update`. This is where our Components update.
9. Update audio, advance scheduler timers with `IncreaseTimers(delta)`, and call the scheduler's `Update`.
10. Call `RenderFrame` to build the snapshot and render it.

The calls worth pointing out are `aGame.Update(*myWorld, delta)` and `myWorld->Update(delta)`. The first is for decisions involving the game as a whole. The second runs behavior attached to individual Actors.

We don't add a call here for every new gameplay class. Attach a Component to an Actor and the World will call it.

## Follow a real Component

Open [Game.cpp](../Source/Application/Game/Game.cpp) at `ConfigureWorld`. The rotation example looks like this:

```cpp
if (Actor* actor = aWorld.FindActor("OrientationGizmos_TGE"))
{
    SpinComponent* rotator = actor->AddComponent<SpinComponent>("WalkthroughRotator");
    rotator->SetDegreesPerSecond(45.0f);
}
```

The level supplies the Actor and its mesh. We find that Actor by its exported name, attach our behavior, and set its speed before it starts. If the Actor isn't in this level, the block is skipped.

`SpinComponent` is the C++ type. `WalkthroughRotator` is the name of this particular Component on the Actor. Component names must be nonempty and unique on their Actor.

Now open [SpinComponent.h](../Source/Application/Game/SpinComponent.h) and [SpinComponent.cpp](../Source/Application/Game/SpinComponent.cpp):

- `BeginPlay` reads the target's existing rotation, so it starts from the orientation in the level.
- `Update` adds `degreesPerSecond * deltaTime` to the yaw and writes the rotation back. At 45 degrees per second, a 0.1-second update adds 4.5 degrees.
- With no target Component name, it rotates the Actor. With a name, it looks for a `SceneComponent` on that Actor and rotates that instead.

One detail to watch in [Transform.h](../Source/Engine/GameFramework/World/Transform.h): rotation is stored as yaw, pitch, roll. The vector's `x` holds yaw around Y; it doesn't mean rotation around X.

Changing the speed in C++ needs a rebuild. Reloading the level doesn't compile code.

## Actor and Component APIs

Open [Actor.h](../Source/Engine/GameFramework/World/Actor.h) and [Component.h](../Source/Engine/GameFramework/World/Component.h). These are the APIs we'll use most:

| Call | What we use it for |
| --- | --- |
| `world.SpawnActor(name)` | Create an Actor in code. Duplicate Actor names get a numeric suffix. |
| `world.FindActor(name)` | Find an Actor by its name. Check the result before using it. |
| `actor.AddComponent<T>(name)` | Create a Component and attach it to the Actor. |
| `actor.GetComponent<T>()` | Find the first Component of that C++ type. |
| `actor.FindComponent(name)` | Find a specific instance by name. Check its type before casting. |
| `actor.GetTransform()` | Read or change the Actor's position, rotation, or scale. |
| `component.GetOwner()` / `GetWorld()` | Reach the Actor or World from a Component. |

An Actor has a transform, but spawning one doesn't make it visible. It needs something like a mesh Component for that. Existing mesh, animation, camera, and light Components are under [GameFramework/Components](../Source/Engine/GameFramework/Components/).

Use `Component` for behavior. Use `SceneComponent` when the Component also needs its own transform relative to the Actor. `SpinComponent` changes an existing transform, so it only needs `Component`.

The owner is assigned when the Component is attached. Don't try to use `GetOwner()` in its constructor.

## Who calls BeginPlay, Update, and EndPlay?

Open [World.cpp](../Source/Engine/GameFramework/World/World.cpp). `BeginPlay` walks the Components and starts each one. `Update` first removes objects marked for destruction, starts any new Components, then updates enabled Components on active Actors. Disabled Components can still receive `BeginPlay`.

The World collects its Component list before calling them. If a Component adds another Component during `BeginPlay` or `Update`, the new one waits until the next World update. Something added in `Game::Update`, before the World collects that list, can start and update in the same frame.

`EndPlay` runs when a started Component is removed or its World is cleared. Use it to undo subscriptions made in `BeginPlay`. For example, a player Component that listens for input should remove its listener here. Otherwise a later input event could call a destroyed object.

The World owns Actors, and Actors own Components. The pointers these APIs return are borrowed: don't delete them. A scene replacement destroys the old objects, so look them up again in the new World.

Calling `Destroy()` marks an Actor or Component for removal and stops its normal updates immediately. The World frees it during cleanup. Gameplay should request destruction and let the World handle the lifecycle; don't call `World::Clear`, `BeginPlay`, or `Update` yourself.

## Where ConfigureWorld fits for now

Today, `ConfigureWorld` is where we manually attach game Components and spawn test Actors. The rotator above is one example. It lets us try behavior before we can create it from the level export.

We want the level JSON to say which Components an Actor has and how they're configured. Built-in scene Components already come through the importer and builder. Custom exported Components currently become placeholders; we still need their type and settings to reach a game-owned Component factory.

With that factory in place, the loading path should be:

```text
Read level JSON
    -> create Actors and Components through factories
    -> apply properties and resolve references
    -> BeginPlay
```

The game registers which C++ class to create for each exported gameplay type. The builder uses those registrations without needing to know every game class itself. We still need to inspect a real custom export before deciding on its fields.

Move the manual attachments into that data as we add support. Test Actors can live in a test level, or be removed when they're no longer needed. Once `ConfigureWorld` is empty, we can remove the method and its call. There's no reason to keep it just in case; if we later need another hook, we can add one for that actual problem.

## Input and changing levels

[InputBindings.json](../Bin/Settings/InputBindings.json) defines the named actions. Startup applies them through `InputSettingsApplier`. `Game::Initialize` adds the `ReloadScene` listener, and `Game::Shutdown` removes it. For input belonging to one Actor, put the listener in its Component's `BeginPlay` and remove it in `EndPlay`.

For an attack, the input listener could set an attack-requested flag. The combat Component would then check that flag and its cooldown in `Update`. Health, combat, enemy AI, and loot Components are things we would write; the World supplies the lifecycle, not those game rules.

To change levels, request the exported file through `GameApplication`:

```cpp
application.RequestSceneLoad("lvl_component_test/Lvl_Component_Test_Level.json");
```

The path is relative to the configured scenes folder. `Game::Initialize` receives the application, so game code can keep a reference if it needs to request another level later.

The request waits until the start of the next frame. If several requests arrive before then, the last one wins. `ReloadCurrentScene` requests the last successfully loaded scene again. To change the startup level, edit `initialScene` in `ApplicationSettings.json`, or request one from `Game::Initialize`.

Reloading builds fresh Actors and Components. It doesn't preserve pointers into the previous World.

## From the World to the screen

Finally, open [WorldRenderer.cpp](../Source/Engine/GameFramework/Rendering/WorldRenderer.cpp). `WorldRenderer::Build` reads the World after gameplay has updated and fills a render snapshot with what needs drawing.

The camera must have begun play, be enabled, and belong to an active Actor. Rendered Components also need to have begun play and be enabled, with an active owner. `RenderFrame` passes the snapshot to the graphics engine.

This is why moving an Actor in a Component's `Update` affects the image drawn that frame: rendering reads the World afterward.

## When something doesn't work

| Problem | Start here |
| --- | --- |
| Input does nothing | The action binding, its listener, then `InputMapper::Update`. |
| Actor lookup returns null | The actual exported name and `World::FindActor`. |
| Component doesn't update | Was it attached? Has it begun play? Is it enabled and its Actor active? |
| Level doesn't change | `RequestSceneLoad`, `myPendingScene`, the requested file path, and the import log. |
| Mesh doesn't show up | Actor and Component state, the active camera, and missing-asset diagnostics. |
| Reload crashes | Pointers or listeners still referring to objects in the old World. |

Useful breakpoints for the walkthrough: `InitializeGameSession`, `ProcessPendingSceneLoad`, `SpinComponent::BeginPlay`, `SpinComponent::Update`, `World::Update`, and `RunMainLoop`.
