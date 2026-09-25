# GameFramework MVP

The MVP has one concrete Game, one GameApplication, one current World, and one
synchronous update loop. It keeps boundaries that carry real data or ownership
for the game AGP runs.

## Start here

Read these files in order:

1. `Source/Application/Game/Main.cpp`: construct Game and GameApplication and call
   `application.Run(game)`.
2. `Source/Application/Game/GameApplication.h/.cpp`: own startup, scene transitions,
   the World, the frame loop, Escape handling, and shutdown.
3. `Source/Application/Game/Game.h/.cpp`: choose the initial scene and add
   project behavior.
4. `Source/Engine/GameFramework/Scenes/SceneData.h`: the importer-to-World data
   boundary.
5. `Source/Engine/GameFramework/Scenes/WorldFromSceneData.h/.cpp`: convert
   SceneData into a candidate World.
6. `Source/Engine/GameFramework/World/World.h`, `Actor.h`, and `Component.h`:
   live ownership and lifecycle.
7. `Source/Engine/GameFramework/Rendering/WorldRenderer.cpp`: copy live World
   values into the renderer snapshot.

## Ownership and roles

```text
Main
├── Game
└── GameApplication
    └── World
        └── Actors
            └── Components

ServiceLocator
├── InputMapper
├── AudioManager
└── AssetRegistry
```

GameApplication owns and coordinates one run. Game owns project-specific choices and
behavior. ServiceLocator owns the shared service objects; GameApplication creates,
updates, and shuts them down in the required order. GraphicsEngine remains a
process singleton that GameApplication drives without owning.

Game also owns one MeshLibrary for its code-configured skeletal demo. It
initializes that library from AssetRegistry's Content root and reuses it whenever
a new World is configured.

Game is concrete. Its runtime-facing methods have only the dependencies they use:

```cpp
class Game
{
public:
    void Initialize(GameApplication& anApplication);
    void ConfigureWorld(World& world);
    void Update(World& world, float deltaTime);
    void Shutdown();
};
```

Only initialization receives GameApplication because Game needs to request the first
scene and register its reload callback. Update and configuration receive the live
World directly. Shutdown does not require a World.

GameApplication exposes three operations to Main and Game:

```cpp
int Run(Game& game);
bool RequestSceneLoad(SceneId sceneId);
bool ReloadCurrentScene();
```

`Run` returns zero after a clean session and reports failure with an exception,
which Main logs before returning one. The two scene methods return whether the
request was accepted.

## Startup and one frame

Startup creates the window and graphics state, installs AssetRegistry,
AudioManager, and InputMapper in ServiceLocator, connects input, and registers
runtime controls. GameApplication then calls Game initialization and consumes the
required initial scene request before showing the window.

Each frame stays on the application thread:

```text
messages and pending scene request
-> resize / timer
-> InputMapper::Update
-> quit check
-> Game::Update
-> World::Update
-> AudioManager::Update
-> WorldRenderer::Build
-> render and present
```

There is no gameplay worker, mailbox, fixed-step layer, or scene-loading thread.
The renderer's shadow workers remain renderer internals.

Escape belongs to GameApplication. Its listener and `WM_QUIT` both set the runtime's
private quit state. Game and Components cannot request process exit.

## Scene loading

Game calls `RequestSceneLoad(SceneId)` or `ReloadCurrentScene()`. Requests are
last-write-wins until the next frame boundary. The runtime captures and clears the
request before it starts loading, so a newer request made during activation stays
queued for the following boundary.

```text
SceneId
-> GameApplication selects the scene file
-> UnrealSceneImporter
-> SceneData
-> BuildWorldFromSceneData
-> candidate World
-> Game::ConfigureWorld
-> replace World
-> BeginPlay
```

Scene selection and material fallback are application concerns. Import conversion
stops at SceneData. The builder owns the fixed mapping from SceneData variants to
live component types.

A failed later load leaves the running World unchanged and consumes that attempt.
It is retried only after another explicit request. A failed initial load is fatal.
An exception after World commit is fatal because there is no rollback layer.

## World and Component behavior

- World owns Actors; Actors own Components with `unique_ptr`.
- Every Actor has a transform. SceneComponent adds a local transform relative to
  its Actor.
- `BeginPlay` runs once for Components present when the World starts.
- Enabled Components on active Actors update in insertion order.
- Additions made during Component callbacks begin updating at the next World
  update. Additions made by Game before `World::Update` are available that frame.
- Destroy marks an object immediately; storage is released at an update boundary
  or when the World clears.
- A Component whose BeginPlay started receives `EndPlay` once.
- Actor and Component pointers are temporary borrows. Do not keep them across
  destruction or scene replacement.

Actor names are unique within a World and Component names are unique within an
Actor. Components can find dependencies by name or type during BeginPlay or
Update. There is no Actor hierarchy, reflection, event bus, physics, streaming,
or general dependency-injection layer.

## Input and cleanup

Runtime-wide bindings such as Escape, debug camera, and renderer views are
installed by GameApplication. Game installs F4/F7/F8 because it owns those actions.
Components listen to named actions and remove their listeners during EndPlay.

Shutdown keeps InputMapper alive until every borrower can unregister:

```text
Game::Shutdown
-> clear World and run Component EndPlay
-> remove runtime listeners and release mouse
-> clear retained render references
-> ServiceLocator::KillServices
-> destroy window/runtime state
```

GameApplication performs the same cleanup after a startup or frame exception. It
preserves the first failure while still attempting later cleanup phases.

## Deliberate scope

Scene loading is synchronous. Delta time is capped at 250 ms. Meshes, materials,
animation, cameras, lights, and render passes reuse the existing graphics engine.
GameApplication calls the concrete Game directly, and SceneData is converted by one
fixed world builder.

Build the Game and relevant test projects in Visual Studio for x64. Runtime checks
should exercise the concrete GameApplication path, especially Escape exit, scene
replacement, failed replacement, input listener cleanup, and service teardown.
