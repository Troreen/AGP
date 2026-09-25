# Engine architecture, the simple version

This page follows the real Game executable from Main to a running World. It skips
most renderer details.

## The whole path

```text
Main constructs Game and GameApplication
    -> GameApplication starts the window and engine services
    -> Game requests the initial SceneId
    -> GameApplication selects a file; UnrealSceneImporter creates SceneData
    -> BuildWorldFromSceneData builds a candidate World
    -> Game adds project-specific behavior
    -> GameApplication commits the World and calls BeginPlay
    -> the frame loop updates Game, World, audio, and rendering
```

There is one concrete game and one concrete runtime, connected by direct method
calls.

## The main pieces

### Main and GameApplication

`Main.cpp` resolves the Content folder, constructs `Game`, constructs
`GameApplication`, and calls `application.Run(game)`.

GameApplication is the coordinator for one run. It owns the window, low-level input
handlers, frame loop, current World, scene-request state, and per-run rendering
state. It calls Game at the few points where project behavior belongs:

- `Initialize(GameApplication&)` initializes the Game-owned MeshLibrary, registers
  game controls, and requests the first scene.
- `ConfigureWorld(World&)` adds code-only behavior before a new World starts.
- `Update(World&, float)` handles per-frame game requests.
- `Shutdown()` releases game listeners and game-owned session work.

GameApplication also owns quitting. Escape and `WM_QUIT` set its private quit flag.
After input is sampled, a quit request leaves the loop before Game, World, audio,
or rendering update again.

### Game

`Game` contains behavior specific to this project: the initial scene choice,
reload input, the demo chest controls, music, and additions made in
`ConfigureWorld`. It borrows the runtime during initialization so its F4 callback
can request a reload. It does not own the World or the services.

Game owns one MeshLibrary for its code-configured skeletal demo. It initializes
the library once from AssetRegistry's Content root and reuses it when configuring
new Worlds.

Scene requests are queued. They are never performed in the middle of input or a
World update. `RequestSceneLoad` keeps the latest request until the next safe
boundary; `ReloadCurrentScene` requests the last scene that committed.

### Scene loading and SceneData

GameApplication maps the game-local `SceneId` to an exported JSON file and calls
`UnrealSceneImporter`. It supplies fallback assets when building the World.

The result is `SceneData`: plain Actor and Component records, not live objects.
`BuildWorldFromSceneData` turns those records into a candidate World. Keeping this
data boundary lets import errors fail before the current World is touched.

If a later candidate cannot be built or configured, the current World stays live.
The failed request is consumed and runs again only after a new explicit request.
The initial scene must succeed because the game cannot start without a World.

### World, Actor, and Component

The live ownership chain is small:

```text
GameApplication owns one World
World owns its Actors
Actor owns its Components
```

An Actor has a name, tags, an active state, and a transform. Components provide
behavior and purpose: mesh, camera, light, spinning, input handling, and so on. A
`SceneComponent` also has a local transform relative to its Actor.

The candidate World receives project behavior through `Game::ConfigureWorld`.
Once configuration succeeds, GameApplication clears the old World, installs the new
one, ensures a camera exists, and calls `World::BeginPlay`. Each begun Component
later receives one matching `EndPlay` when it is removed or the World is cleared.

### Services and the runtime

`ServiceLocator` is both the shared access point and the owner of three services:

- InputMapper translates device state into named actions.
- AudioManager controls sound and music.
- AssetRegistry finds and loads Content assets.

GameApplication creates these services and gives ownership to ServiceLocator. It then
coordinates their use. The distinction matters: GameApplication owns the run and
ordering; ServiceLocator owns the service objects and deletes them.

InputMapper borrows `InputHandler` and `XInputHandler` from GameApplication. During
shutdown, Game removes its listeners first, the World is cleared so Components
can remove theirs, runtime listeners are removed, and only then does
ServiceLocator delete InputMapper, AudioManager, and AssetRegistry.

GraphicsEngine remains a process singleton. GameApplication initializes and drives it
but does not own its storage. WorldRenderer is the narrow bridge that reads a live
World and builds the renderer snapshot.

## Following startup to BeginPlay

1. Windows enters `wWinMain`.
2. Main constructs Game and GameApplication and calls `application.Run(game)`.
3. GameApplication creates the window, initializes graphics, and installs the three
   ServiceLocator-owned services.
4. Runtime input is connected. Escape, debug camera, and render-view controls are
   registered by GameApplication.
5. GameApplication calls `Game::Initialize`. Game initializes its MeshLibrary from
   the configured Content root, registers F4/F7/F8, starts music, and requests the
   Blockout scene.
6. The runtime captures that request, selects its exported file, and calls
   `UnrealSceneImporter` to produce SceneData.
7. `BuildWorldFromSceneData` creates Actors and Components in a candidate World.
8. `Game::ConfigureWorld` adds project-only behavior.
9. GameApplication commits the candidate, adds a debug camera if the scene has no
   active camera, and calls `World::BeginPlay`.
10. The normal frame loop begins.

Each frame samples input once, calls Game, updates the World and audio, builds a
render snapshot, and presents it. A scene requested during that work waits for the
next scene boundary.

## Where to add something

- Put a new level object or starting value in the exported scene.
- Write a Component for reusable behavior that updates with an Actor.
- Use `Game::ConfigureWorld` for a project-specific connection between scene
  objects.
- Put mesh, material, and texture files under Content and refer to them by their
  Content-relative paths.
- Bind a global key once in the runtime or Game that owns it; Components listen to
  named actions and remove their listeners in `EndPlay`.

Keep scene description as data, live behavior in Components, and run ordering in
GameApplication.
