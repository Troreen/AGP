# Engine map

For the gameplay-facing overview, start with
[GameFrameworkMVP.md](GameFrameworkMVP.md). Paths below are relative to the
repository root.

## Program ownership

Windows enters `wWinMain` in `Source/Application/Game/Main.cpp`. Main resolves
the executable-relative Content folder, constructs one concrete `Game` and one
`GameApplication`, and calls `application.Run(game)`.

`GameApplication` is the application coordinator. It owns the window, platform input
handlers, frame loop, current World, pending/current scene IDs, and per-run render
state. It borrows Game for the duration of `Run`. Game owns project behavior and
chooses the initial scene; it does not own the loop or World.

The remaining ownership is direct:

```text
wWinMain
├── Game
└── GameApplication
    ├── window and platform input handlers
    ├── current World
    │   └── Actors
    │       └── Components
    └── per-run render and scene-request state

ServiceLocator
├── InputMapper
├── AudioManager
└── AssetRegistry

GraphicsEngine::Get()
└── process-wide renderer singleton
```

GameApplication creates and coordinates the three services, but ServiceLocator owns
them and deletes them through `KillServices`. InputMapper borrows the runtime's
`InputHandler` and `XInputHandler`, so the runtime keeps those handlers alive
until service shutdown. GameApplication drives GraphicsEngine but does not own its
singleton storage.

## Startup and shutdown

Startup follows one concrete path:

1. GameApplication creates the Win32 window and initializes GraphicsEngine.
2. It creates AssetRegistry, AudioManager, and InputMapper and gives their
   ownership to ServiceLocator.
3. It connects platform input and installs runtime controls. Escape belongs to
   GameApplication and requests loop exit.
4. It calls `Game::Initialize(GameApplication&)`. Game initializes its owned
   MeshLibrary from AssetRegistry's Content root, registers F4/F7/F8 behavior,
   starts music, and requests the initial scene.
5. GameApplication loads that scene synchronously, asks Game to configure the
   candidate World, commits it, ensures a camera exists, and calls `BeginPlay`.
6. It shows the window and enters the frame loop.

On shutdown, Game removes its listeners, the World is cleared while services are
still available, runtime listeners and render references are cleared, and
ServiceLocator deletes its services. The window and remaining runtime state are
destroyed afterward. This order lets Component `EndPlay` callbacks unregister
from InputMapper safely.

Cleanup also runs after startup or frame failures. Game shutdown is attempted at
most once after Game initialization has begun, and the original failure remains
the one reported to Main.

## Frame sequence

All gameplay runs on the application thread. Scene requests are processed only
at frame boundaries.

```text
window messages / WM_QUIT
-> process one pending scene request
-> resize or skip a minimized frame
-> update timer and clamp delta time
-> InputMapper::Update
-> stop immediately if Escape requested quit
-> apply runtime camera input
-> Game::Update(World&, deltaTime)
-> World::Update(deltaTime)
-> AudioManager::Update(deltaTime)
-> WorldRenderer::Build
-> GraphicsEngine render / execute / present
```

Escape and `WM_QUIT` set the same private runtime quit state. Game and Components
do not receive a quit API. Input callbacks record scene or gameplay requests;
the larger operation happens after input dispatch.

The renderer can record shadow passes on worker threads and joins them before
playback. Gameplay has no worker, mailbox, or snapshot queue.

## Scene flow

Game can call `RequestSceneLoad(SceneId)` or `ReloadCurrentScene()` through the
GameApplication reference received during initialization. The pending request is
last-write-wins until the next scene boundary. At that boundary the runtime
captures and clears one request before loading it.

```text
SceneId
-> GameApplication selects the scene file
-> UnrealSceneImporter
-> SceneData
-> BuildWorldFromSceneData
-> candidate World
-> Game::ConfigureWorld
-> commit World / ensure camera / BeginPlay
```

Scene ID-to-file mapping and the fallback asset choice live in GameApplication.
The importer converts the export format into `SceneData`. `BuildWorldFromSceneData`
maps those records to live Actors and Components and binds fallback assets when needed.

A later pre-commit load failure leaves the current World running and does not
retry until Game makes another request. An initial load failure is fatal because
there is no playable World. A failure after commit is also fatal; the runtime
does not keep a rollback copy.

## Rendering sequence

WorldRenderer copies camera/light properties, mesh/material bindings, Actor
transforms, and skeletal joint poses into a render snapshot. A missing camera
produces an empty frame. GraphicsEngine then performs its existing frame work:

| Phase | Inputs and output |
| --- | --- |
| Resource preparation | Creates missing mesh buffers and refreshes material data before concurrent reads. |
| `BuildShadowJobs()` | Selects casters and shadow maps; fills the light buffer with matching shadow assignments. |
| `RecordAndExecuteShadows()` | Records shadow command lists and plays them back in order; worker failure falls back to serial recording. |
| `PrepareSceneCommands()` | Clears targets and binds camera constants, samplers, environment, and shadow resources. |
| `RenderGBuffer()` | Opaque geometry writes surface data and depth. |
| `RenderAmbientOcclusion()` | Reads GBuffer positions and normals and writes screen-space AO. |
| `RenderDeferredLighting()` | Reads GBuffer, AO, and shadows, then composites linear lighting to the back buffer. |
| `RenderDebugView()` | Optionally replaces the composite with a selected diagnostic view. |
| `RenderTransparentGeometry()` | Draws blended elements back-to-front with the opaque depth buffer and light data. |

Pass helpers rely on this order and shared bindings. Resource unbinding beside
each pass prevents read/write conflicts. CPU statistics measure preparation,
shadow recording, and scene recording rather than GPU execution time.

## Subsystem locations

| Location | Responsibility |
| --- | --- |
| `Source/Application/Game` | Main, concrete Game, GameApplication, scene selection/preparation, controls, and game materials |
| `Source/Engine/GameFramework/World` | World, Actor, Component, and Transform ownership and lifecycle |
| `Source/Engine/GameFramework/Components` | Cameras, lights, meshes, and scene-local transforms |
| `Source/Engine/GameFramework/Scenes` | SceneData and conversion into a live World |
| `Source/Engine/GameFramework/UnrealSceneImporter` | Unreal export parsing and data conversion |
| `Source/Engine/GameFramework/AssetHandling` | Shared asset registry and asset types |
| `Source/Engine/GameFramework/Rendering` | World-to-render-snapshot adapter |
| `Source/Engine/GraphicsEngine` | Frame orchestration, resources, materials, RHI, and shaders |
| `Source/Utilities` | Startup helpers, logging, and utility glue |
| `CommonUtilities/include` | Shared math, input, timer, and utility types |

See [EngineOptimisations.md](EngineOptimisations.md) for renderer switches,
build commands, scheduling details, and visual acceptance checks.

## Readability conventions

Use `// --- Phase name ---` only for major responsibilities in large files. Keep
ownership and unusual ordering beside the implementation that depends on them.
Prefer a direct call and a clear type name over a forwarding wrapper.

The root `.clang-format` uses Allman braces, four-column tab indentation, and a
140-column limit. `.editorconfig` supplies matching settings for owned C++ files.
Always brace control-flow bodies, use explicit lambda captures, and use `struct`
for data rather than types with behavior. Keep unrelated formatting separate from
behavior changes.
