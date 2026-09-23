# GameFramework architecture refactoring plan

Status: planning only. This document describes a target architecture and an ordered migration. It does not implement the refactor.

The governing question is: **what is the smallest, clearest architecture that correctly supports the game AGP is actually building?** AGP runs one concrete game. This plan therefore removes extension points whose only purpose is supporting interchangeable games or hypothetical engine consumers. It retains boundaries that carry real behavior: scene import data, live World construction, World/Actor/Component ownership, rendering, input, audio, assets, and platform runtime work.

The plan is based on repository state `f9874ed`. The worktree was clean during investigation. No implementation build or runtime test is claimed as passing for this plan; the verification baseline is described in section 11.

## 1. Current architecture and ownership flow

### Program entry and runtime ownership

```text
Windows wWinMain                                  Main.cpp:11-50
├── stack Game                                   Main.cpp:34
├── stack GameScene                              Main.cpp:35
└── temporary GameApplication                    Main.cpp:36-39
    └── automatic GameApplication::Impl          GameApplication.cpp:446-449
        ├── borrows IGame&
        ├── owns copied Config and SceneSource
        ├── owns GameContext
        │   └── owns unique_ptr<World>
        ├── owns ComponentRegistry
        ├── owns HWND and platform input handlers
        ├── owns render command/snapshot/overlay state
        └── owns debug-camera host state
```

`Main.cpp` resolves the executable-relative Content folder, constructs `Game` and `GameScene`, and passes a lambda to `GameApplication::Run` solely to forward to `GameScene::Load` (`Source/Application/Game/Main.cpp:22-39`). `GameApplication` itself has no instance state. Its `Run` method immediately constructs `GameApplication::Impl`, which owns every real per-run resource (`Source/Engine/GameFramework/Runtime/GameApplication.h:32-51`, `.cpp:55-111,446-449`).

`GameContext` owns the current `World` and also stores the pending/current scene, content root, client size, quit flag, and scene-request gate (`Runtime/GameContext.h:8-59`). `GameApplication` is its friend and directly mutates that state throughout the loop. The Context is therefore both a game-facing facade and the runtime's private state bag.

### Engine-wide services and rendering

```text
Process lifetime
├── GraphicsEngine::Get()               process singleton
└── ServiceLocator::GetInstance()        process singleton and service owner
    ├── InputMapper                      owned during a run
    ├── AudioManager                     owned during a run
    └── AssetRegistry                    owned during a run
```

`ServiceLocator` owns all three registered services. Its setters transfer raw-pointer ownership and delete replacements; `KillServices` deletes input, audio, then assets (`GameFramework/ServiceLocator.h:20-39`, `.cpp:18-55`). This is the intended architecture and remains central. Runtime code creates and initializes the services (`Runtime/GameApplication.cpp:186-219`), updates input/audio, and shuts them down. The `InputMapper` borrows runtime-owned `InputHandler` and `XInputHandler` objects, so those platform adapters must outlive service shutdown.

`GraphicsEngine` is a separate process singleton (`GraphicsEngine.cpp:454-458`). The runtime initializes and drives it but does not own its storage or explicitly destroy it. `WorldRenderer` is a useful narrow adapter: it reads a live World into a render snapshot and leaves GPU orchestration to `GraphicsEngine` (`GameFramework/Rendering/WorldRenderer.cpp:8-68`).

### Current startup, frame, scene, and shutdown path

Startup (`Runtime/GameApplication.cpp:113-287`):

1. Create the Win32 window and associate its user data with the runtime-owned `InputHandler` (`:153-172`).
2. Canonicalize Content root; initialize `GraphicsEngine`; create the game command list (`:173-183`).
3. Create and initialize AssetRegistry, AudioManager, and InputMapper in ServiceLocator (`:186-219`).
4. Connect platform input and install host controls/listeners (`:221-268`).
5. Call `Game::Initialize`; the concrete Game installs F4/F7/F8 input, queues Blockout, and starts music (`Game.cpp:122-155`).
6. Load the queued initial scene, or BeginPlay an empty pre-created World if no request exists (`GameApplication.cpp:270-281`).
7. Show/focus the window (`:281-286`).

Frame order (`Runtime/GameApplication.cpp:289-381`):

1. Pump Win32 messages and convert `WM_QUIT` to the runtime quit flag.
2. Process one pending scene at a safe frame boundary.
3. Skip the frame while the client area is zero; resize when needed.
4. Update and clamp delta time.
5. Update InputMapper once.
6. Recenter mouse look and apply the deferred debug-camera toggle.
7. Call `Game::Update`.
8. Call `World::Update`.
9. Call `AudioManager::Update`.
10. Build the snapshot, render, execute, and present.

Scene loading currently follows this chain:

```text
GameContext::LoadScene
  -> optional pending SceneType
  -> GameApplication::LoadPendingScene
  -> nullable SceneSource std::function
  -> Main lambda
  -> GameScene::Load
  -> UnrealSceneImporter
  -> SceneData
  -> ComponentRegistry::CreateWorld
  -> Game::ConfigureWorld
  -> replace World / debug camera / BeginPlay
  -> optional IGame callbacks
```

Relevant anchors are `Runtime/GameContext.cpp:7-20`, `Runtime/GameApplication.cpp:393-444`, `Scenes/SceneData.h:140-147`, `Application/Game/GameScene.cpp:25-117`, and `Scenes/ComponentRegistry.cpp:77-237`.

Normal shutdown closes scene requests, calls `Game::Shutdown`, clears the World, removes host listeners, releases the mouse, and kills ServiceLocator services (`Runtime/GameApplication.cpp:113-150,384-391`). Clearing the World before deleting the mapper is important because Component `EndPlay` callbacks can remove input listeners. `Impl` then destroys the HWND (`:64-70`). Failures after game startup attempt `Game::Shutdown` once, clear the World, kill services, and rethrow.

## 2. Biggest sources of unnecessary complexity

### A generic game contract for one concrete game

`IGame` provides six virtual hooks, four with default no-op implementations (`Runtime/IGame.h:7-41`). The only production implementation is `Game` (`Application/Game/Game.h:9-22`). Other implementations are test fixtures and the hypothetical `PublicGameplayConsumer`. There is no runtime selection between games.

This interface obscures the actual dependency while making the engine host appear reusable. Removing it lets the runtime call concrete game methods directly and makes the one-game constraint visible in code.

### A context that duplicates runtime state

`GameContext` owns World but the runtime controls every meaningful transition through friendship. It also forwards scene loading, reload, quit, content root, and client size. Production Game only needs scene control during initialization/input callbacks and World during update/shutdown. Passing the whole Context to every callback expands the visible API without defining a real responsibility.

It also contains invalid state: `myCurrentSceneType` has no initializer, but `ReloadScene` reads it directly (`GameContext.h:53-54`, `.cpp:17-20`). A comment states a precondition instead of representing “no current scene” safely.

### Two runtime classes with one lifetime

`GameApplication` is a stateless facade around an automatic `GameApplication::Impl`. The nested class is the only real owner. This is not a persistent heap PImpl, and public ABI compatibility is not required. The extra shell makes readers inspect two types to discover one lifetime.

The public header also exposes `RenderPassNotificationTimer` only so a unit test can instantiate a private implementation detail (`GameApplication.h:11-30`; `GameFrameworkTests.cpp:542-552`). `GetFontResource` merely forwards the already-public `FontAsset::GetFont()` (`GameApplication.cpp:451-454`; `FontAsset.h:13-16`).

### Scene forwarding and misleading wrappers

`SceneSource` has one production callable, a lambda in Main that forwards directly to `GameScene::Load`. `GameScene` is not a scene; it is a thin stateful wrapper around game-specific scene-id mapping, Unreal import, and material fallback. Its `MeshLibrary` initialization path is currently commented out (`GameScene.cpp:25-46`).

`ComponentRegistry` does not register anything. It is a stateless, closed `std::visit` builder from `SceneData` to `World` (`ComponentRegistry.cpp:77-237`). Its name and object lifetime imply extensibility that does not exist.

### Surprising scene-request behavior

Current transitions contain four hard-to-reason-about policies:

- `LoadScene` is last-write-wins, but `LoadPendingScene` does not capture and consume the request at attempt start.
- A callback can overwrite `myPendingScene` during construction, after the display name was captured; commit can then label candidate A as scene B (`GameApplication.cpp:395-432`).
- A later pre-commit failure retains the request and retries it every frame, repeating work, logging, and failure callbacks (`:415-425`).
- Requests accepted during `BeginPlay` or `OnSceneLoaded` are silently erased by the final `pending.reset()` (`:435-443`).

### State and APIs retained for tests rather than the game

The nullable SceneSource and empty-world startup path are used by the text-overlay test, while the shipped Game always supplies a source and requests an initial scene. `OnSceneLoaded` and `OnSceneLoadFailed` are used by test fixtures but not the concrete Game. `PublicGameplayConsumer` validates a generic reusable engine contract that the project explicitly does not need.

Tests should preserve required behavior, not force production interfaces to remain generic.

## 3. Proposed target architecture

```text
wWinMain
  ├── constructs concrete Game
  ├── constructs concrete GameRuntime(config)
  └── GameRuntime::Run(game)
      ├── owns main loop and platform window/input adapters
      ├── owns current World and scene-request state
      ├── calls free LoadGameSceneData
      │   └── Unreal import -> SceneData
      ├── calls free BuildWorld(SceneData, assets, clientSize)
      ├── calls concrete Game lifecycle methods
      ├── coordinates ServiceLocator
      │   ├── owns InputMapper
      │   ├── owns AudioManager
      │   └── owns AssetRegistry
      ├── drives WorldRenderer
      └── drives process-singleton GraphicsEngine
```

Put the concrete `GameRuntime` in `Source/Application/Game`. That location is a deliberate dependency decision: runtime can call the one concrete `Game` without making the engine library depend upward on application code and without replacing `IGame` with callbacks, templates, or another interface.

The prompt's `Main -> Game -> GameRuntime` sketch is exploratory, not a requirement to add a forwarding method. The clearest actual call is `runtime.Run(game)`: Main shows both concrete objects, and the type named Runtime visibly owns the run. A one-line `Game::Run` that merely constructs or forwards to GameRuntime would hide the true owner and add no responsibility.

The intended concrete interaction is:

```cpp
class Game
{
public:
    void Initialize(GameRuntime& runtime);
    void ConfigureWorld(World& world);
    void Update(World& world, float deltaTime);
    void Shutdown();
};

SceneData LoadGameSceneData(SceneId scene, const std::filesystem::path& contentRoot,
                            AssetRegistry& assets);
```

Only `Initialize` receives runtime control because the actual Game F4 input callback captures Runtime to request reload and Game chooses the initial scene. Components do not receive a scene-control API. Update and configuration receive the narrower World reference they actually use. Shutdown receives no World: initialization or the initial scene can fail before a live World exists, and clearing the active camera is redundant because Runtime clears the World immediately afterward.

`LoadGameSceneData` is a game-local free function implemented in `GameSceneLoading.cpp`. It needs no Game state, so making it a Game method or granting Runtime friendship would only obscure that fact.

`GameRuntime` should be non-copyable and non-movable while running because the HWND stores a pointer to its `InputHandler`. It owns runtime state directly. Do not replace `Impl` with a differently named PImpl or generic “state” object solely to hide includes. The class is application-private, so concrete platform dependencies are acceptable.

Header direction stays simple: Main includes `Game.h` and `GameRuntime.h`, constructs `GameRuntime::Config`, then calls `runtime.Run(game)`. `GameRuntime.h` forward-declares `Game`; `Game.h` forward-declares `GameRuntime` and `World`; implementation files include both definitions. GameFramework never includes an application header.

## 4. Class and file disposition

| Current class/file | Disposition | Reason / target |
| --- | --- | --- |
| `Application/Game/Main.cpp` | Retain and reduce | Resolve config, construct `Game` and `GameRuntime`, call `runtime.Run(game)`, return 0 on success, and keep top-level exception logging. Remove GameScene and forwarding lambda. |
| `Application/Game/Game.h/.cpp` | Retain concrete | Remove `IGame` inheritance/overrides. Keep concrete initialize/update/configure/shutdown behavior. `Initialize` receives Runtime, update/configure receive World, and shutdown receives no World. Do not add a forwarding `Run`. |
| `Application/Game/GameScene.h/.cpp` | Delete wrapper | Move the real responsibility into game-local `GameSceneLoading.h/.cpp` as free `LoadGameSceneData`. This keeps Game.cpp readable without creating another class or friendship. |
| `Application/Game/MeshLibrary.*` | Retain | The code-configured skeletal demo still uses it (`Game.cpp:183-198`). Remove only incidental ownership by GameScene. Confirm no required constructor side effect before deleting that member. |
| `Runtime/IGame.h` | Delete | Only one production Game exists. Do not replace it with another interface or callback table. |
| `Runtime/GameContext.h/.cpp` | Delete | Fold World and runtime state into GameRuntime; expose only direct operations needed by concrete Game. |
| `Runtime/GameApplication.h/.cpp` | Replace/move | Create application-owned `GameRuntime.h/.cpp`; merge outer shell, Impl, and Context state. |
| `RenderPassNotificationTimer` | Reduce to cpp-local detail | It does not belong in a public runtime header. Preserve its overlay behavior and remove the implementation-detail unit test. |
| `GameApplication::GetFontResource` | Delete | Call public `FontAsset::GetFont()` directly; no AssetHandling implementation change. |
| `Scenes/SceneData.h` records | Retain | `SceneData` is a meaningful importer-to-builder DTO and keeps source schema separate from live objects. |
| `SceneType`, `GetSceneFile`, `GetSceneName` in `SceneData.h:12-58` | Move/rename | These are game content choices. Define game-local `SceneId` plus one mapping table/switch in scene-loading code. |
| `SceneLoadContext`, `SceneSource` in `SceneData.h:140-147` | Delete | Pass the few concrete values directly. Remove the callback/lambda layer. |
| `Scenes/ComponentRegistry.*` | Rename/reduce | Replace the stateless class with free `BuildWorld(const SceneData&, AssetRegistry&, Vector2u)` in `SceneWorldBuilder.*`. Keep the closed visitor; do not add a factory map. |
| `UnrealSceneImporter` | Retain unchanged | Import/conversion is real. Changing its class form adds churn without clarifying the runtime. |
| `World`, `Actor`, `Component`, `Transform`, `SceneComponent` | Retain | They express real ownership, lifecycle, and spatial responsibilities. |
| `WorldRenderer` | Retain unchanged | It is the real World-to-render-snapshot boundary; its current `Build` name is sufficient. |
| `ServiceLocator` | Retain as sole engine-wide service owner | Keep InputMapper, AudioManager, and AssetRegistry here. Runtime initializes, updates, and calls the existing `KillServices`. Retain the current setters/raw owned storage in this refactor; do not introduce DI or another service facade. |
| `GraphicsEngine` | Retain singleton | Runtime coordinates active use but does not own singleton storage. Do not wrap it in ServiceLocator during this refactor. |
| `AssetHandling/*` | No implementation edits | Thomas owns this folder. Adapt only external call sites if his independent API changes require it. |
| `PublicGameplayConsumer.cpp/.vcxproj` | Delete | The generic external-game contract no longer exists. Keep the separate public-header isolation script for actual remaining GameFramework headers. |
| Checked-in `.vcxproj`, filters, Premake | Update atomically with moves | GameFramework explicitly lists old Runtime files; Game must list new GameRuntime/scene-loading files. Premake globs alone do not update checked-in projects. |

## 5. Intended ownership and lifetime of major systems

| Object/system | Owner | Lifetime and borrowing rule |
| --- | --- | --- |
| `Game` | Automatic local in `wWinMain` | Borrowed by `GameRuntime::Run`; outlives all input callbacks registered by Game. |
| `GameRuntime` | Automatic local in `wWinMain` | Owns one complete run: initialization, loop, optional live World, and shutdown. Non-movable while HWND exists. |
| Current `World` | `GameRuntime` via nullable `unique_ptr` | Absent until initial scene commit; replaced only at a scene boundary. Owns Actors; Actors own Components. Cleared before services are deleted. |
| Candidate `World` | Scene transition local | Exists only during pre-commit build/configuration. Destruction before BeginPlay requires no EndPlay. |
| HWND | `GameRuntime` | Created before graphics/services; destroyed after normal/exception cleanup while InputHandler is still alive. |
| `InputHandler`, `XInputHandler` | `GameRuntime` | Borrowed by ServiceLocator-owned InputMapper. Must outlive mapper shutdown. |
| `InputMapper` | `ServiceLocator` | Created for a run. Game/components/runtime remove listeners before deletion. |
| `AudioManager` | `ServiceLocator` | Created for a run; updated after World; destructor releases SoundEngine. |
| `AssetRegistry` | `ServiceLocator` | Created for a run; borrowed by game scene loading and BuildWorld. No new owner wrapper. |
| `GraphicsEngine` | Process singleton | Initialized/driven by GameRuntime; storage persists to static teardown. No ownership claim by runtime. |
| `WorldRenderer` | Stateless namespace/class boundary | Called by GameRuntime to build snapshots; owns no World or GraphicsEngine. |
| `SceneData` | Value local during load | Returned by import/game preparation, consumed by BuildWorld, then discarded. |
| Render command/snapshot/overlay/debug camera host state | `GameRuntime` | Per-run state. It must not create a second owner for assets or World. |

Service shutdown order remains explicit and readable:

```text
Game::Shutdown() (no World dependency)
-> clear World if present (Component EndPlay removes listeners)
-> remove runtime input listeners / release mouse
-> clear render snapshot / notification widget / retained font asset
-> ServiceLocator::KillServices (InputMapper, AudioManager, AssetRegistry)
-> destroy window and remaining runtime state
```

The explicit render-reference reset ensures the snapshot, notification widget, and font do not retain shared asset objects beyond AssetRegistry shutdown. ServiceLocator's raw ownership API is imperfect but understood and unrelated to the surrounding architecture. Leave its storage, setter names, getters, and `KillServices` unchanged in this refactor.

## 6. Intended runtime flows

### Startup

1. `wWinMain` resolves Content root and fills `GameRuntime::Config`.
2. `wWinMain` constructs concrete Game and GameRuntime, then calls `runtime.Run(game)`.
3. GameRuntime borrows Game for the duration of `Run`, creates the window, and initializes GraphicsEngine.
4. GameRuntime installs AssetRegistry, AudioManager, and InputMapper in ServiceLocator.
5. GameRuntime connects platform input and installs host controls.
6. GameRuntime marks Game initialization as begun, then calls `Game::Initialize(runtime)`.
7. Game binds game controls, starts music, and queues the required initial `SceneId`.
8. GameRuntime consumes and loads that initial request synchronously.
9. The candidate World is imported, built, game-configured, committed, given a fallback debug camera if needed, and begun.
10. Only then is the window shown and the frame loop entered.

There is no nullable scene source. The shipped Game must select an initial scene. An initial pre-commit failure is fatal because no playable World exists.

### Frame

Preserve the existing order unless a separate behavior change proves necessary:

```text
messages / quit
-> consume one pending scene request
-> resize or skip minimized frame
-> timer and clamped delta
-> InputMapper::Update
-> mouse recenter / host debug-camera request
-> Game::Update(World&, delta)
-> World::Update(delta)
-> AudioManager::Update(delta)
-> WorldRenderer::Build
-> GraphicsEngine render / execute / present
```

Game may request a future scene during input/gameplay. Runtime never clears or replaces World in the middle of input or World dispatch.

Game's complete runtime-control surface is deliberately small:

- `RequestScene(SceneId)`: store one last-write-wins request for the next scene boundary; reject `None`/invalid ids and requests after loop shutdown begins.
- `ReloadCurrentScene()`: request the current committed scene and return false if no scene has committed yet.

Only Game receives `RequestScene` and `ReloadCurrentScene`. It does not need a public `RequestQuit`; Win32 messages own normal quit, and the compile-time test options enforce bounded test runs internally. Components continue to interact with their World and services; they do not receive a scene-transition API. Game input callbacks that capture `GameRuntime&` are registered after Runtime construction, removed by `Game::Shutdown()`, and cannot outlive `GameRuntime::Run`.

### Scene transition

At the single-threaded frame boundary, consume the current request:

```cpp
const std::optional<SceneId> requested = std::exchange(myPendingScene, std::nullopt);
if (!requested)
{
    return;
}
const SceneId scene = *requested;
```

Use the captured concrete `scene` for file selection, logging, candidate construction, and `myCurrentScene`. Any later request writes into the now-empty pending slot and remains queued for the next boundary. Multiple requests before the next boundary remain last-write-wins. There is no synchronization claim: window messages, input, gameplay, and scene loading all run on the application thread.

Pre-commit:

1. `LoadGameSceneData(scene, contentRoot, services.GetAssetRegistry())` imports and applies game-specific asset fallback policy.
2. `BuildWorld(sceneData, assets, clientSize)` creates a candidate World.
3. `Game::ConfigureWorld(candidate)` adds game-only behavior.

If pre-commit fails:

- On the initial load, log and propagate; normal startup cleanup follows.
- On a later load, log once, destroy the candidate, preserve the live World, and leave the consumed request cleared. Retry requires an explicit new request.
- A newer request queued while handling the attempt remains pending.

Commit and activation:

1. Clear the old World while services still exist.
2. Move in the candidate and set current scene from the captured concrete `scene`.
3. Reset/ensure the debug camera.
4. Call `World::BeginPlay`.

A failure after commit is fatal because the old World is gone. Do not add rollback machinery. A request queued during activation remains pending for the next boundary; it is never silently erased.

### Shutdown and exceptions

On normal exit, stop accepting loop work and call `Game::Shutdown()` once. Whether shutdown succeeds or throws, Runtime then clears the World if present, removes host listeners, releases the mouse, clears snapshot/widget/font references, and calls `ServiceLocator::KillServices`. A normal `Game::Shutdown()` exception becomes the primary failure, is rethrown after resource cleanup, and reaches Main.

If startup or the loop already has a primary exception after Game initialization began, Runtime attempts `Game::Shutdown()` once. A secondary shutdown or cleanup exception is logged while the original exception remains the one rethrown. Early platform/graphics/service failures before Game initialization begins skip Game shutdown but still clean all partial Runtime/service state. Cleanup code must tolerate `myWorld == nullptr`, because Initialize or the initial pre-commit load can fail before the first World commits.

Implement cleanup as explicit best-effort phases rather than one chain that aborts on the first cleanup exception. Each remaining phase is still attempted. The first failure becomes the primary exception when no earlier failure exists; later cleanup failures are logged, so World, listeners, render references, services, and window all receive their cleanup opportunity.

Keep top-level logging in Main. Use `void GameRuntime::Run(Game&)` with exceptions as the failure channel; Main returns 0 after success and 1 after catching an exception. The current `Run` always returns 0, so an `int` return adds no information (`GameApplication.cpp:150`).

## 7. Scene-loading simplification

The target scene path is:

```text
captured SceneId
-> free LoadGameSceneData in GameSceneLoading.cpp
-> UnrealSceneImporter::ImportScene
-> prepare game-specific material fallback through AssetRegistry
-> SceneData value
-> free BuildWorld
-> candidate World
-> Game::ConfigureWorld
-> commit / camera / BeginPlay
```

Keep these boundaries:

- **Game scene selection and fallback policy** are game-specific and live in the free `LoadGameSceneData` function in `Source/Application/Game/GameSceneLoading.cpp`.
- **Unreal parsing/conversion** remains an importer responsibility.
- **SceneData** remains a plain value representation between import and live objects.
- **BuildWorld** remains the closed mapping from SceneData records to Actor/Component instances.
- **World** owns live lifecycle after construction.

Remove these layers:

- no `SceneSource` callback;
- no Main forwarding lambda;
- no generic `SceneLoadContext` bundle;
- no stateful `GameScene` wrapper;
- no `ComponentRegistry` object when a free build function expresses the work;
- no registration/factory framework for the fixed component set.

`BuildWorld` may need narrow access to authored Actor metadata currently granted through `friend class ComponentRegistry`. Rename that friendship to the builder only if necessary. Prefer legitimate narrow setters for authored active/tags/archetype data when they improve the object API; do not create a builder interface or generic mutation channel.

## 8. Explicit decisions for the named abstractions

| Abstraction | Decision | Why |
| --- | --- | --- |
| `IGame` | Remove | One concrete Game; virtual hooks exist for reuse/tests, not runtime behavior. |
| `GameContext` | Remove | It duplicates GameRuntime state and forwards operations. World ownership becomes visible on GameRuntime. |
| `GameApplication` | Replace with `GameRuntime` | The name “runtime” describes the loop/world owner directly. |
| `GameApplication::Impl` | Merge into GameRuntime | One lifetime should have one visible owner; no ABI/PImpl requirement. |
| `SceneSource` | Remove | One lambda forwards one concrete load operation. |
| `SceneLoadContext` | Remove | Pass content root and AssetRegistry explicitly where required; client size belongs at BuildWorld. |
| `GameScene` | Remove as a class | Retain its real load/preparation responsibility as a game-local free function in `GameSceneLoading.*`. |
| `ComponentRegistry` | Replace with free `BuildWorld` | It is a stateless closed-set builder, not a registry. |
| `SceneData` | Retain | It is a real data boundary and supports candidate construction. |
| `UnrealSceneImporter` | Retain unchanged | Source-format conversion is real. Changing its class form adds churn without helping the runtime architecture. |
| `World/Actor/Component` | Retain | They encode clear ownership and lifecycle. |
| `WorldRenderer` | Retain | It separates World traversal from graphics execution. |
| `ServiceLocator` | Retain intentionally and unchanged internally | It is the required central owner/access point for input, audio, and assets. Its raw owned pointers, setters/getters, and `KillServices` are outside this refactor. |
| `GraphicsEngine` singleton | Retain | No evidence justifies a new owner/service abstraction in this refactor. |

## 9. AssetHandling boundary and Thomas handoff

Do not edit `Source/Engine/GameFramework/AssetHandling` as part of this refactor.

The preferred target requires no new AssetHandling abstraction. Keep its use localized to three places:

1. GameRuntime creates and initializes AssetRegistry through ServiceLocator.
2. `LoadGameSceneData` resolves authored/fallback materials and reads useful error information.
3. `BuildWorld` resolves mesh/material assets while constructing components.

Expected boundary, subject to Thomas's independent implementation:

- initialize from Content root;
- resolve/get a typed asset by identifier;
- distinguish “not found” from other failures where fallback behavior depends on it;
- provide a useful diagnostic.

If Thomas changes method names or return/error representation, only these external call sites should adapt. Do not request DI, an asset-provider interface, a callback wrapper, or a second registry. `GameApplication::GetFontResource` can disappear immediately because `FontAsset::GetFont()` is already public; that does not require an AssetHandling edit.

The current source/tests also show pre-existing API drift that must not be blamed on this refactor: framework tests refer to fields such as `StaticMeshData::Mesh` and material `.Parent`, while current SceneData uses `MeshName` and material `Name` (`SceneData.h:78-89`; reported test anchors `GameFrameworkTests.cpp:241,462-468`). The baseline step below must identify the actual expected API in the implementation checkout, fix only external callers/tests to that API, and record any remaining blocker. It does not require an unrelated rebase or waiting for Thomas when the checked-out boundary is sufficient.

## 10. Ordered implementation sequence

Each stage should be reviewable and buildable on its own. Do not have parallel writers edit the runtime owner simultaneously. Do not add temporary bridges, new virtual hooks, or callbacks merely to split a cross-cutting change.

### Stage 0 — Establish the build and API baseline

Files: external AssetHandling callers and tests only when they demonstrably drift from the API present in the implementation checkout; build notes.

- Run clean Game, GameFrameworkTests, and GameRuntimeTests builds in a normal allowed environment.
- Record pre-existing build/runtime failures before changing architecture.
- Resolve external test/caller drift such as stale `StaticMeshData::Mesh`/material `.Parent` references against the actual expected API.
- Do not edit `AssetHandling`, mandate an unrelated rebase, or wait for Thomas unless the actual checkout lacks a usable boundary.

Verification: a recorded clean baseline, or a short explicit blocker list that separates pre-existing failures from later refactor results.

### Stage 1 — Move the runtime mechanically into the application

Files: `Runtime/GameApplication.*` -> `Application/Game/GameRuntime.*`, Main include/call site, Game/GameFramework project and filter files, Premake, GameRuntimeTests project, timer test.

- Move and rename the runtime owner into the Game application, flattening the outer shell and `Impl` only where that can remain behavior-preserving.
- Temporarily retain `IGame`, `GameContext`, `SceneSource`, and the existing GameScene lambda contract. This stage changes dependency location and visible ownership, not gameplay behavior.
- Preserve window/input borrows, exact frame order, scene behavior, cleanup policy, ServiceLocator ownership, and GraphicsEngine singleton behavior.
- Make the overlay timer cpp-local and remove its implementation-detail unit test at `GameFrameworkTests.cpp:542-552` in this same stage. Remove the font forwarding helper.
- Update `GameRuntimeTests.vcxproj`, which directly compiles application sources rather than linking the WindowedApp: add `GameRuntime.cpp`, keep references only to GameFramework/Graphics/CommonUtilities/Logger and external libraries, and do not add a Game executable project reference.

Verification: existing runtime scenarios behave identically; hidden/visible startup, minimized skip/resize, input once per frame, overlay, debug camera, audio, D3D diagnostics, and exception cleanup still pass.

### Stage 2 — Replace ComponentRegistry with the concrete builder

Files: `Scenes/ComponentRegistry.*` -> `Scenes/SceneWorldBuilder.*`, Actor access, framework tests, project/filter files.

- Introduce free `BuildWorld` with the existing closed visitor.
- Preserve candidate construction, diagnostics, active-camera validation, and missing-asset behavior.
- Remove the runtime's registry member.

Verification: every SceneData variant builds the same component; malformed actor/component rejects the candidate; missing mesh/material keeps its existing skip/fallback contract; camera rules remain.

### Stage 3 — Atomically make the runtime concrete and simplify scene loading

Files: `Game.h/.cpp`, `GameRuntime.*`, `SceneData.h`, remove `IGame.h` and `GameContext.*`, remove `GameScene.*`, add `GameSceneLoading.h/.cpp`, Main, runtime tests, PublicGameplayConsumer target/files, project/filter/Premake entries.

- Change Main to construct `Game` and `GameRuntime::Config`, construct GameRuntime, and call `runtime.Run(game)`.
- Remove Game inheritance/virtual dispatch, `IGame`, `GameContext`, SceneSource, SceneLoadContext, the Main lambda, and the stateful GameScene wrapper together. Do not introduce a transitional interface, callback, or engine-to-application include.
- Add free game-local `LoadGameSceneData`; move SceneId/path/display mapping into application code; keep SceneData, importer, and BuildWorld boundaries.
- Fold World, optional current/pending SceneId, client size, content root, and quit state directly into GameRuntime.
- Add only `RequestScene` and `ReloadCurrentScene` to the Game-facing Runtime API. Actual Game input callbacks capture Runtime and remove those captures during `Game::Shutdown()`; components receive neither operation.
- Apply the final transition policy here, once: consume one captured concrete id at the single-threaded boundary, preserve a newer pending request, do not auto-retry a failed later candidate, and make initial pre-commit/post-commit failures fatal.
- Make `Game::Shutdown()` independent of World. Unify normal/exception cleanup so a shutdown throw still clears World/render references/services and is rethrown, while a secondary cleanup throw preserves the earlier primary failure.
- Clear World, then host listeners/mouse, then snapshot/widget/font references, then call existing `ServiceLocator::KillServices` before Runtime/window destruction.
- Delete `PublicGameplayConsumer.cpp` and `PublicGameplayConsumer.vcxproj` (and any solution entry dedicated to that hypothetical consumer). Update `RunPublicHeaderIsolation.ps1` to remove the deleted `Runtime` folder from its folder list; it continues to compile the GameFramework headers that actually remain.
- Confirm that removing GameScene's MeshLibrary member removes no required initialization side effect; retain MeshLibrary for the concrete skeletal demo.

`GameRuntimeTests.vcxproj` continues to compile the application implementation directly. Replace `GameScene.cpp` with `GameRuntime.cpp` and `GameSceneLoading.cpp`; keep direct Game/Spin/MeshLibrary/PrimitiveMeshBuilder sources and lower-library references. Define `AGP_RUNTIME_TESTS` only for this target. Under that macro, compile one small concrete `RuntimeTestOptions` structure with exactly: an initial SceneId, a maximum frame count, at most one requested transition plus its phase, and a small failure-point enum. It may select representative initialize/load/configure/BeginPlay-update/shutdown failures, but it must not contain callbacks, scripts, a fake Game, or a generic framework. No test API is present in production builds.

Verification: entry/lifecycle order; initial scene and one later transition; reload; later failure preserves old World, reports once, and leaves no implicit retry; request queued during activation survives; listener removal; BeginPlay/EndPlay counts; representative initialize, activation/update, and shutdown failures; deterministic quit at max frames; actual Blockout/chest imports and material fallback; no AssetHandling edits.

### Stage 4 — Documentation and final project cleanup

Update `README.md`, `Source/Engine/GameFramework/README.md`, `Docs/Architecture.md`, `Docs/EngineArchitectureBasics.md`, `Docs/GameFrameworkMVP.md`, `Docs/InputMapperAndServiceLocator.md`, and `Docs/ImporterHandoff.md`. Mark old audit/refactoring plans superseded if they remain. Regenerate or manually update checked-in Visual Studio projects and filters consistently.

Verification: repository search finds no live `IGame`, `GameContext`, `GameApplication`, `SceneSource`, `SceneLoadContext`, `GameScene`, or `ComponentRegistry` references outside intentionally retained historical documents.

The refactor ends after Stage 4 and its verification. Changing UnrealSceneImporter into a free function or modernizing ServiceLocator storage/names is explicitly out of scope; those changes do not improve the chosen runtime architecture enough to justify more churn.

## 11. Risks and required verification

### Baseline limitation

No implementation tests were run or claimed passed for this planning task. A normal MSBuild attempt from the investigation environment reached compilation but was blocked by Visual Studio FileTracker `E_ACCESSDENIED` under the filesystem sandbox. The investigators did not use a destructive or broad-permission workaround. The approved historical `/t:ModelViewer` command is not a valid target for the current solution. In addition, the test/source AssetHandling API drift described in section 9 may cause independent compile failures.

Before implementation, establish a clean baseline with an allowed normal build environment and record any failures as pre-existing. Do not use an older executable as evidence for new source.

### Risk and verification matrix

| Risk | Required verification |
| --- | --- |
| Moving runtime to Application accidentally creates Engine -> Application dependency | Inspect project references/includes: Game depends on GameFramework; GameFramework must not include Game headers. |
| A new callback/interface is introduced to recover test injection | Architecture review: GameRuntime calls concrete Game; test-only faults remain compiled only in the test target. |
| Input mapper outlives borrowed platform handlers | Verify Runtime is non-movable; mapper is deleted before InputHandler/XInputHandler; run focus loss, mouse look, camera, F4/F7/F8, F5/F6. |
| World clears after services and EndPlay touches dead input/assets | Assert shutdown order; test components unregister listeners during EndPlay; inspect locator is empty afterward. |
| Scene identity changes during load | Queue another request during an attempt; assert committed current id is the captured id and newer request runs next. |
| Failed later scene retries endlessly | Inject one candidate failure; assert one load/log, old World remains, and pending is empty. |
| Request during activation is lost | Queue during activation in the focused test path; assert it remains pending for next boundary. |
| Initial failure leaves partial services/window/world | Run invalid-initial and initialize-failure paths; assert one appropriate Game shutdown attempt, service cleanup, and no begun component leak. |
| Post-commit BeginPlay/update failure leaks lifecycle | Assert every begun Component gets exactly one EndPlay and destruction; original exception survives cleanup. |
| Removing GameScene's MeshLibrary changes importer setup | Trace and test actual FBX/scene import before deleting the member; retain MeshLibrary for the skeletal demo. |
| Builder conversion changes scene behavior | Cover every ComponentRecord alternative, transforms/tags/active state, active-camera uniqueness, asset fallback/skip diagnostics. |
| Render order or singleton assumptions change | Run visible and hidden runtime scenarios, minimized resize, overlay, scene rendering, and inspect D3D debug diagnostics. |
| Thomas's AssetHandling work collides | Keep edits outside AssetHandling; adapt only the three external boundary call sites to the actual implementation checkout and record a blocker only if that boundary is unusable. |
| Documentation continues to teach the removed architecture | Update the named authoritative docs in the same final stage and mark superseded plans clearly. |

Intended build/test matrix after each relevant stage:

- Debug and Release Game builds;
- GameFrameworkTests after builder/import changes;
- GameRuntimeTests scenarios covering sample/chest content, text overlay, camera/render-pass controls, invalid initial load, and representative initialize, activation/update, and shutdown failures through the one bounded test-options structure;
- actual staged Content run for imported scenes and material fallback;
- public/header smoke for the APIs that remain intentionally supported;
- D3D diagnostic queue inspection where available.

## 12. Final simplicity and readability audit checklist

### Entry and ownership

- [ ] Starting at Main, a reader reaches concrete Game and concrete GameRuntime without an interface or forwarding lambda.
- [ ] GameRuntime visibly owns the loop, current World, pending/current scene state, window, and per-run platform/render state.
- [ ] ServiceLocator visibly owns InputMapper, AudioManager, and AssetRegistry.
- [ ] GraphicsEngine is clearly documented as a process singleton coordinated, not owned, by GameRuntime.
- [ ] World -> Actor -> Component remains the sole live gameplay ownership chain.

### Runtime flow

- [ ] Startup order is readable in one function or one short sequence of meaningfully named phase methods.
- [ ] Frame order is readable without jumping through Context or callback wrappers.
- [ ] Shutdown order shows Game, World, listener, service, and window cleanup directly.
- [ ] Exception cleanup attempts Game shutdown at most once and preserves the original failure.

### Scene flow

- [ ] There is one optional pending request and one explicit optional/current scene value.
- [ ] A boundary consumes one captured request before loading.
- [ ] New requests remain queued for the next boundary; last write wins.
- [ ] Recoverable candidate failure reports once and never auto-retries.
- [ ] Initial pre-commit and post-commit failures are explicitly fatal.
- [ ] SceneId/file mapping is game-local.
- [ ] The path is direct: game-local load -> importer -> SceneData -> BuildWorld -> candidate -> commit.
- [ ] No `SceneSource`, `SceneLoadContext`, stateful GameScene, or fake registry remains.

### Abstractions

- [ ] Every retained class has state, lifecycle, or a meaningful responsibility.
- [ ] No new manager, provider, DI layer, callback table, generic container, or factory map replaces removed code.
- [ ] SceneData/importer and World/Actor/Component boundaries remain because they separate real concerns.
- [ ] WorldRenderer remains the narrow rendering adapter.
- [ ] AssetHandling internals remain externally owned and untouched.

### API and tests

- [ ] Game methods receive Runtime, World, or no argument according to actual need.
- [ ] No uninitialized current-scene or impossible nullable service/source state remains in the valid run path.
- [ ] Tests cover actual runtime behavior and bounded failure injection without recreating IGame.
- [ ] PublicGameplayConsumer source/project are deleted; public-header isolation covers only supported GameFramework headers.
- [ ] Project files, filters, Premake, and authoritative docs match the final file layout.

## 13. Independent audit and revision record

The completed draft was independently audited from four perspectives and revised before delivery:

- **Simplicity / unnecessary abstraction:** removed the proposed one-line `Game::Run`, chose direct `runtime.Run(game)`, made scene loading a free game-local function, and excluded optional importer/ServiceLocator cleanup.
- **Readability / newcomer tracing:** specified the two-operation Game-facing scene API, the lifetime of Runtime-capturing input callbacks, nullable pre-initial-World state, concrete SceneId consumption, and deletion of PublicGameplayConsumer.
- **Ownership / lifecycle correctness:** removed World from Game shutdown, made World cleanup a Runtime responsibility, added render-reference release before AssetRegistry deletion, and specified best-effort cleanup with primary-exception preservation.
- **Implementation feasibility / scope:** moved the runtime mechanically before the atomic contract removal, documented direct-compilation of app sources in GameRuntimeTests, bounded test-only controls, header/project direction, timer-test removal, and the baseline handling of external AssetHandling API drift.

The refactor is complete only when a new programmer can answer, from Main and GameRuntime alone, where the program starts, who owns the loop and World, how a scene is loaded, how Game participates, where input/audio/assets come from, and who shuts every major system down.
