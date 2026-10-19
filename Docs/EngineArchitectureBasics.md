# Engine architecture, the simple version

This page follows the real Game executable from Main to a running World. It skips
most renderer details.

## The whole path

```text
Main initializes services, then constructs Game and GameApplication
    -> GameApplication starts the window and engine services
    -> GameApplication takes the initial scene path from settings or a Game request
    -> UnrealSceneImporter creates SceneData
    -> BuildWorld builds a candidate World
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

- `Initialize(GameApplication&)` registers game controls and can request a scene.
- `ConfigureWorld(World&)` adds code-only behavior before a new World starts.
- `Update(World&, float)` handles per-frame game requests.
- `Shutdown()` releases game listeners and game-owned session work.

GameApplication also owns quitting. Escape and `WM_QUIT` set its private quit flag.
After input is sampled, a quit request leaves the loop before Game, World, audio,
or rendering update again.

### Game

`Game` contains behavior specific to this project: reload input, music, and additions made in
`ConfigureWorld`. It borrows the runtime during initialization so its F4 callback
can request a reload. It does not own the World or the services.

Game owns one MeshLibrary that keeps the FBX importer initialized while assets load.

Scene requests are queued. They are never performed in the middle of input or a
World update. `RequestSceneLoad` keeps the latest request until the next safe
boundary; `ReloadCurrentScene` requests the last scene that committed.

### Scene loading and SceneData

GameApplication resolves the requested scene path under the configured Content
scene folder and calls `UnrealSceneImporter`. It supplies fallback assets when
building the World.

The result is `SceneData`: plain Actor and Component records, not live objects.
`BuildWorld` turns those records into a candidate World. Keeping this
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

`ServiceLocator` is the shared access point and owner of engine services, including:

- InputMapper translates device state into named actions.
- AudioManager controls sound and music.
- AssetRegistry finds and loads Content assets.
- EventBus delivers typed gameplay events, including collision overlaps.
- EngineSettings, AnimationManager, and coroutine services support runtime systems.

Main calls `ServiceLocator::Initialize` and initializes settings, assets,
animation, and audio before starting GameApplication. GameApplication coordinates
the run and cleanup; ServiceLocator owns the service objects.

InputMapper borrows `InputHandler` and `XInputHandler` from GameApplication. During
shutdown, Game removes its listeners first, the World is cleared so Components
can remove theirs, runtime listeners are removed, and only then does
ServiceLocator cleans up services.

GraphicsEngine remains a process singleton. GameApplication initializes and drives it
but does not own its storage. `World::GenerateRenderSnapshot` gathers borrowed
camera, mesh, and light Component pointers for `GraphicsEngine::Render`.

## Following startup to BeginPlay

1. Windows enters `wWinMain`.
2. Main initializes services, constructs Game and GameApplication, and calls `application.Run(game)`.
3. GameApplication creates the window and initializes graphics.
4. Runtime input is connected. Escape, debug camera, and render-view controls are
   registered by GameApplication.
5. GameApplication calls `Game::Initialize`, which registers F4 reload.
6. The runtime takes a Game-requested scene or the `initialScene` setting and calls
   `UnrealSceneImporter` to produce SceneData.
7. `BuildWorld` creates Actors and Components in a candidate World.
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
