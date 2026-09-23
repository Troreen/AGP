# GameFramework architecture refactoring plan

Status: implemented and verified in the current source in Debug and Release; this document remains the design rationale and ordered migration record.

The governing question is: **what is the smallest, clearest architecture that correctly supports the game AGP is actually building?** AGP runs one concrete game. This plan therefore removes extension points whose only purpose is supporting interchangeable games or hypothetical engine consumers. It retains boundaries that carry real behavior: scene import data, live World construction, World/Actor/Component ownership, rendering, input, audio, assets, and platform runtime work.

This is a responsibility plan, not a mechanical file map. The implementation may place a helper or split a file differently when the resulting code is simpler and the ownership/dependency direction remains clear. **When choosing between preserving an old abstraction and updating its callers, prefer updating the callers if the resulting architecture is simpler.**

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

Tests should preserve required game/runtime behavior, not force production interfaces to remain generic. Remove or rewrite tests whose only purpose is preserving the old generic-runtime shape. Do not create an elaborate test-only interface, context, callback system, scripted harness, or substitute Game merely to keep those testing patterns. Prefer tests that exercise the concrete Game and GameApplication. Add a tiny test-target-only control only when a behavior cannot otherwise be made deterministic and the control is materially smaller than the abstraction it replaces.

## 3. Proposed target architecture

```text
wWinMain
  ├── constructs concrete Game
  ├── constructs concrete GameApplication(config)
  └── GameApplication::Run(game)
      ├── owns main loop and platform window/input adapters
      ├── owns current World and scene-request state
      ├── calls free LoadAndPrepareGameSceneData
      │   └── Unreal import -> SceneData
      ├── calls free BuildWorldFromSceneData(SceneData, assets, clientSize)
      ├── calls concrete Game lifecycle methods
      ├── coordinates ServiceLocator
      │   ├── owns InputMapper
      │   ├── owns AudioManager
      │   └── owns AssetRegistry
      ├── drives WorldRenderer
      └── drives process-singleton GraphicsEngine
```

Put the concrete `GameApplication` in `Source/Application/Game`. That location is a deliberate dependency decision: runtime can call the one concrete `Game` without making the engine library depend upward on application code and without replacing `IGame` with callbacks, templates, or another interface.

The prompt's `Main -> Game -> GameApplication` sketch is exploratory, not a requirement to add a forwarding method. The clearest actual call is `application.Run(game)`: Main shows both concrete objects, and `GameApplication` visibly owns the run. A one-line `Game::Run` that merely constructs or forwards to GameApplication would hide the true owner and add no responsibility.

The intended concrete interaction is:

```cpp
class Game
{
public:
    void Initialize(GameApplication& anApplication);
    void ConfigureWorld(World& world);
    void Update(World& world, float deltaTime);
    void Shutdown();
};

SceneData LoadAndPrepareGameSceneData(SceneId scene, const std::filesystem::path& contentRoot,
                                      AssetRegistry& assets);
```

Only `Initialize` receives runtime control because the actual Game F4 input callback captures Runtime to request reload and Game chooses the initial scene. Components do not receive a scene-control API. Update and configuration receive the narrower World reference they actually use. Shutdown receives no World: initialization or the initial scene can fail before a live World exists, and clearing the active camera is redundant because Runtime clears the World immediately afterward.

`LoadAndPrepareGameSceneData` is a game-local free function implemented in `GameSceneLoading.cpp`. Its name makes both operations visible: it imports the selected scene and applies the game's material-fallback preparation. It needs no Game state, so making it a Game method or granting Runtime friendship would only obscure that fact.

`GameApplication` should be non-copyable and non-movable while running because the HWND stores a pointer to its `InputHandler`. It owns runtime state directly. Do not replace `Impl` with a differently named PImpl or generic “state” object solely to hide includes. The class is application-private, so concrete platform dependencies are acceptable.

Header direction stays simple: Main includes `Game.h` and `GameApplication.h`, constructs `GameApplication::Config`, then calls `application.Run(game)`. `GameApplication.h` forward-declares `Game`; `Game.h` forward-declares `GameApplication` and `World`; implementation files include both definitions. GameFramework never includes an application header.

## 4. Class and file disposition

The paths below communicate ownership and dependency direction. They are proposed placements, not a requirement to preserve every filename or helper boundary. During implementation, prefer fewer, clearer units when an obvious simplification appears; do not move code across the Engine/Application boundary or blur ownership merely to reduce file count.

| Current class/file | Disposition | Reason / target |
| --- | --- | --- |
| `Application/Game/Main.cpp` | Retain and reduce | Resolve config, construct `Game` and `GameApplication`, call `application.Run(game)`, return 0 on success, and keep top-level exception logging. Remove GameScene and forwarding lambda. |
| `Application/Game/Game.h/.cpp` | Retain concrete | Remove `IGame` inheritance/overrides. Keep concrete initialize/update/configure/shutdown behavior. `Initialize` receives Runtime, update/configure receive World, and shutdown receives no World. Do not add a forwarding `Run`. |
| `Application/Game/GameScene.h/.cpp` | Delete wrapper | Move the real responsibility into game-local `GameSceneLoading.h/.cpp` as free `LoadAndPrepareGameSceneData`. This keeps Game.cpp readable without creating another class or friendship. |
| `Application/Game/MeshLibrary.*` | Retain | The code-configured skeletal demo still uses it (`Game.cpp:183-198`). Remove only incidental ownership by GameScene. Confirm no required constructor side effect before deleting that member. |
| `Runtime/IGame.h` | Delete | Only one production Game exists. Do not replace it with another interface or callback table. |
| `Runtime/GameContext.h/.cpp` | Delete | Fold World and runtime state into GameApplication; expose only direct operations needed by concrete Game. |
| `Runtime/GameApplication.h/.cpp` | Replace/move | Create application-owned `GameApplication.h/.cpp`; merge outer shell, Impl, and Context state. |
| `RenderPassNotificationTimer` | Reduce to cpp-local detail | It does not belong in a public runtime header. Preserve its overlay behavior and remove the implementation-detail unit test. |
| `GameApplication::GetFontResource` | Delete | Call public `FontAsset::GetFont()` directly; no AssetHandling implementation change. |
| `Scenes/SceneData.h` records | Retain | `SceneData` is a meaningful importer-to-builder DTO and keeps source schema separate from live objects. |
| `SceneType`, `GetSceneFile`, `GetSceneName` in `SceneData.h:12-58` | Move/rename | These are game content choices. Define game-local `SceneId` plus one mapping table/switch in scene-loading code. |
| `SceneLoadContext`, `SceneSource` in `SceneData.h:140-147` | Delete | Pass the few concrete values directly. Remove the callback/lambda layer. |
| `Scenes/ComponentRegistry.*` | Rename/reduce | Replace the stateless class with free `BuildWorldFromSceneData(const SceneData&, AssetRegistry&, Vector2u)` in `WorldFromSceneData.*`. Keep the closed visitor; do not add a factory map. |
| `UnrealSceneImporter` | Retain unchanged | Import/conversion is real. Changing its class form adds churn without clarifying the runtime. |
| `World`, `Actor`, `Component`, `Transform`, `SceneComponent` | Retain | They express real ownership, lifecycle, and spatial responsibilities. |
| `WorldRenderer` | Retain unchanged | It is the real World-to-render-snapshot boundary; its current `Build` name is sufficient. |
| `ServiceLocator` | Retain as sole engine-wide service owner | Keep InputMapper, AudioManager, and AssetRegistry here. Runtime initializes, updates, and calls the existing `KillServices`. Retain the current setters/raw owned storage in this refactor; do not introduce DI or another service facade. |
| `GraphicsEngine` | Retain singleton | Runtime coordinates active use but does not own singleton storage. Do not wrap it in ServiceLocator during this refactor. |
| `AssetHandling/*` | No implementation edits | Thomas owns this folder. Adapt only external call sites if his independent API changes require it. |
| `PublicGameplayConsumer.cpp/.vcxproj` | Delete | The generic external-game contract no longer exists. Keep the separate public-header isolation script for actual remaining GameFramework headers. |
| Checked-in `.vcxproj`, filters, Premake | Update atomically with moves | GameFramework explicitly lists old Runtime files; Game must list new GameApplication/scene-loading files. Premake globs alone do not update checked-in projects. |

### Naming pass

Names should state purpose and ownership at the call site. Long, precise names are acceptable; hidden responsibility and context-dependent names are not. Apply this as a focused pass over code touched by the refactor, without renaming stable unrelated APIs.

Use these semantic names unless the implementation reveals a clearer equivalent:

- `LoadAndPrepareGameSceneData`, not `LoadGameSceneData`, because the function both imports and applies game-specific fallback preparation.
- `BuildWorldFromSceneData`, not `BuildWorld`, because the source and conversion responsibility should be obvious without opening the function.
- `ProcessPendingSceneLoad`, not `LoadPendingScene`, because it consumes a request, handles failure policy, and may commit a new World.
- `RequestSceneLoad`, `myPendingSceneId`, and `myCurrentSceneId`, so requests and identities cannot be confused with SceneData or live World objects.
- `InitializeInputAndApplicationControls` and `RemoveApplicationInputListeners`, so Escape, debug-camera, and render-pass bindings are visibly owned by GameApplication rather than by the game or an unspecified “host.”
- If test-only types are materially necessary, prefix them with `GameApplication` and name their precise role, such as `GameApplicationFailurePoint`; avoid generic names such as `Options`, `Context`, `Hook`, or `Callback`.

Names such as `GameApplication`, `WorldFromSceneData.*`, `ReloadCurrentScene`, and `WorldRenderer::Build` already communicate their scope sufficiently and should not be churned merely for consistency.

## 5. Intended ownership and lifetime of major systems

| Object/system | Owner | Lifetime and borrowing rule |
| --- | --- | --- |
| `Game` | Automatic local in `wWinMain` | Borrowed by `GameApplication::Run`; outlives all input callbacks registered by Game. |
| `GameApplication` | Automatic local in `wWinMain` | Owns one complete run: initialization, loop, optional live World, and shutdown. Non-movable while HWND exists. |
| Current `World` | `GameApplication` via nullable `unique_ptr` | Absent until initial scene commit; replaced only at a scene boundary. Owns Actors; Actors own Components. Cleared before services are deleted. |
| Candidate `World` | Scene transition local | Exists only during pre-commit build/configuration. Destruction before BeginPlay requires no EndPlay. |
| HWND | `GameApplication` | Created before graphics/services; destroyed after normal/exception cleanup while InputHandler is still alive. |
| `InputHandler`, `XInputHandler` | `GameApplication` | Borrowed by ServiceLocator-owned InputMapper. Must outlive mapper shutdown. |
| `InputMapper` | `ServiceLocator` | Created for a run. Game/components/runtime remove listeners before deletion. |
| `AudioManager` | `ServiceLocator` | Created for a run; updated after World; destructor releases SoundEngine. |
| `AssetRegistry` | `ServiceLocator` | Created for a run; borrowed by game scene loading and `BuildWorldFromSceneData`. No new owner wrapper. |
| `GraphicsEngine` | Process singleton | Initialized/driven by GameApplication; storage persists to static teardown. No ownership claim by runtime. |
| `WorldRenderer` | Stateless namespace/class boundary | Called by GameApplication to build snapshots; owns no World or GraphicsEngine. |
| `SceneData` | Value local during load | Returned by import/game preparation, consumed by `BuildWorldFromSceneData`, then discarded. |
| Render command/snapshot/overlay/debug camera host state | `GameApplication` | Per-run state. It must not create a second owner for assets or World. |

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

1. `wWinMain` resolves Content root and fills `GameApplication::Config`.
2. `wWinMain` constructs concrete Game and GameApplication, then calls `application.Run(game)`.
3. GameApplication borrows Game for the duration of `Run`, creates the window, and initializes GraphicsEngine.
4. GameApplication installs AssetRegistry, AudioManager, and InputMapper in ServiceLocator.
5. GameApplication connects platform input and installs application controls, including an Escape listener that requests loop exit.
6. GameApplication marks Game initialization as begun, then calls `Game::Initialize(application)`.
7. Game binds game controls, starts music, and queues the required initial `SceneId`.
8. GameApplication consumes and loads that initial request synchronously.
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
-> process Escape quit; leave loop before gameplay/render if requested
-> mouse recenter / Runtime debug-camera request
-> Game::Update(World&, delta)
-> World::Update(delta)
-> AudioManager::Update(delta)
-> WorldRenderer::Build
-> GraphicsEngine render / execute / present
```

Game may request a future scene during input/gameplay. Runtime never clears or replaces World in the middle of input or World dispatch.

Game's complete runtime-control surface is deliberately small:

- `RequestSceneLoad(SceneId)`: store one last-write-wins request for the next scene boundary; reject `None`/invalid ids and requests after loop shutdown begins.
- `ReloadCurrentScene()`: request the current committed scene and return false if no scene has committed yet.

Only Game receives `RequestSceneLoad` and `ReloadCurrentScene`. GameApplication owns quit control: it binds Escape as an application input action, registers the corresponding application listener, and sets its internal quit flag when Escape is pressed. After `InputMapper::Update`, GameApplication observes that flag and leaves the loop before Game update, World update, audio update, or rendering. `WM_QUIT` sets the same flag before the frame starts. Game receives no quit method, and components receive neither quit nor scene-transition control. Game input callbacks that capture `GameApplication&` are registered after application construction, removed by `Game::Shutdown()`, and cannot outlive `GameApplication::Run`.

### Scene transition

At the single-threaded frame boundary, consume the current request:

```cpp
const std::optional<SceneId> requested = std::exchange(myPendingSceneId, std::nullopt);
if (!requested)
{
    return;
}
const SceneId scene = *requested;
```

Use the captured concrete `scene` for file selection, logging, candidate construction, and `myCurrentSceneId`. Any later request writes into the now-empty pending slot and remains queued for the next boundary. Multiple requests before the next boundary remain last-write-wins. There is no synchronization claim: window messages, input, gameplay, and scene loading all run on the application thread.

Pre-commit:

1. `LoadAndPrepareGameSceneData(scene, contentRoot, services.GetAssetRegistry())` imports and applies game-specific asset fallback policy.
2. `BuildWorldFromSceneData(sceneData, assets, clientSize)` creates a candidate World.
3. `Game::ConfigureWorld(candidate)` adds game-only behavior.

If pre-commit fails:

- On the initial load, log and propagate; normal startup cleanup follows.
- On a later load, log once, destroy the candidate, preserve the live World, and leave the consumed request cleared. Retry requires an explicit new request.
- A newer request queued while handling the attempt remains pending.

Commit and activation:

1. Clear the old World while services still exist.
2. Move in the candidate and set `myCurrentSceneId` from the captured concrete `scene`.
3. Reset/ensure the debug camera.
4. Call `World::BeginPlay`.

A failure after commit is fatal because the old World is gone. Do not add rollback machinery. A request queued during activation remains pending for the next boundary; it is never silently erased.

### Shutdown and exceptions

On normal exit, stop accepting loop work and call `Game::Shutdown()` once. Whether shutdown succeeds or throws, Runtime then clears the World if present, removes Game and Runtime input listeners (including Escape), releases the mouse, clears snapshot/widget/font references, and calls `ServiceLocator::KillServices`. A normal `Game::Shutdown()` exception becomes the primary failure, is rethrown after resource cleanup, and reaches Main.

If startup or the loop already has a primary exception after Game initialization began, Runtime attempts `Game::Shutdown()` once. A secondary shutdown or cleanup exception is logged while the original exception remains the one rethrown. Early platform/graphics/service failures before Game initialization begins skip Game shutdown but still clean all partial Runtime/service state. Cleanup code must tolerate `myWorld == nullptr`, because Initialize or the initial pre-commit load can fail before the first World commits.

Implement cleanup as explicit best-effort phases rather than one chain that aborts on the first cleanup exception. Each remaining phase is still attempted. The first failure becomes the primary exception when no earlier failure exists; later cleanup failures are logged, so World, listeners, render references, services, and window all receive their cleanup opportunity.

Keep top-level logging in Main. Use `void GameApplication::Run(Game&)` with exceptions as the failure channel; Main returns 0 after success and 1 after catching an exception. The current `Run` always returns 0, so an `int` return adds no information (`GameApplication.cpp:150`).

## 7. Scene-loading simplification

The target scene path is:

```text
captured SceneId
-> free LoadAndPrepareGameSceneData in GameSceneLoading.cpp
-> UnrealSceneImporter::ImportScene
-> prepare game-specific material fallback through AssetRegistry
-> SceneData value
-> free BuildWorldFromSceneData
-> candidate World
-> Game::ConfigureWorld
-> commit / camera / BeginPlay
```

Keep these boundaries:

- **Game scene selection and fallback policy** are game-specific and live in the free `LoadAndPrepareGameSceneData` function in `Source/Application/Game/GameSceneLoading.cpp`.
- **Unreal parsing/conversion** remains an importer responsibility.
- **SceneData** remains a plain value representation between import and live objects.
- **BuildWorldFromSceneData** remains the closed mapping from SceneData records to Actor/Component instances.
- **World** owns live lifecycle after construction.

Remove these layers:

- no `SceneSource` callback;
- no Main forwarding lambda;
- no generic `SceneLoadContext` bundle;
- no stateful `GameScene` wrapper;
- no `ComponentRegistry` object when a free build function expresses the work;
- no registration/factory framework for the fixed component set.

`BuildWorldFromSceneData` may need narrow access to authored Actor metadata currently granted through `friend class ComponentRegistry`. Rename that friendship to the builder only if necessary. Prefer legitimate narrow setters for authored active/tags/archetype data when they improve the object API; do not create a builder interface or generic mutation channel.

## 8. Explicit decisions for the named abstractions

| Abstraction | Decision | Why |
| --- | --- | --- |
| `IGame` | Remove | One concrete Game; virtual hooks exist for reuse/tests, not runtime behavior. |
| `GameContext` | Remove | It duplicates GameApplication state and forwards operations. World ownership becomes visible on GameApplication. |
| Legacy engine-owned `GameApplication` facade | Move and simplify as the application-owned `GameApplication` | The concrete class directly owns the loop and World instead of hiding them in an engine-side facade. |
| `GameApplication::Impl` | Merge into GameApplication | One lifetime should have one visible owner; no ABI/PImpl requirement. |
| `SceneSource` | Remove | One lambda forwards one concrete load operation. |
| `SceneLoadContext` | Remove | Pass content root and AssetRegistry explicitly where required; client size belongs at `BuildWorldFromSceneData`. |
| `GameScene` | Remove as a class | Retain its real load/preparation responsibility as a game-local free function in `GameSceneLoading.*`. |
| `ComponentRegistry` | Replace with free `BuildWorldFromSceneData` | It is a stateless closed-set builder, not a registry. |
| `SceneData` | Retain | It is a real data boundary and supports candidate construction. |
| `UnrealSceneImporter` | Retain unchanged | Source-format conversion is real. Changing its class form adds churn without helping the runtime architecture. |
| `World/Actor/Component` | Retain | They encode clear ownership and lifecycle. |
| `WorldRenderer` | Retain | It separates World traversal from graphics execution. |
| `ServiceLocator` | Retain intentionally and unchanged internally | It is the required central owner/access point for input, audio, and assets. Its raw owned pointers, setters/getters, and `KillServices` are outside this refactor. |
| `GraphicsEngine` singleton | Retain | No evidence justifies a new owner/service abstraction in this refactor. |

## 9. AssetHandling boundary and Thomas handoff

Do not edit `Source/Engine/GameFramework/AssetHandling` as part of this refactor.

The preferred target requires no new AssetHandling abstraction. Keep its use localized to three places:

1. GameApplication creates and initializes AssetRegistry through ServiceLocator.
2. `LoadAndPrepareGameSceneData` resolves authored/fallback materials and reads useful error information.
3. `BuildWorldFromSceneData` resolves mesh/material assets while constructing components.

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

- Run clean Game, GameFrameworkTests, and GameApplicationTests builds in a normal allowed environment.
- Record pre-existing build/runtime failures before changing architecture.
- Resolve external test/caller drift such as stale `StaticMeshData::Mesh`/material `.Parent` references against the actual expected API.
- Do not edit `AssetHandling`, mandate an unrelated rebase, or wait for Thomas unless the actual checkout lacks a usable boundary.

Verification: a recorded clean baseline, or a short explicit blocker list that separates pre-existing failures from later refactor results.

### Stage 1 — Move the runtime mechanically into the application

Files: `Runtime/GameApplication.*` -> `Application/Game/GameApplication.*`, Main include/call site, Game/GameFramework project and filter files, Premake, GameApplicationTests project, timer test.

- Move and rename the runtime owner into the Game application, flattening the outer shell and `Impl` only where that can remain behavior-preserving.
- Temporarily retain `IGame`, `GameContext`, `SceneSource`, and the existing GameScene lambda contract. This stage changes dependency location and visible ownership, not gameplay behavior.
- Preserve window/input borrows, exact frame order, scene behavior, cleanup policy, ServiceLocator ownership, and GraphicsEngine singleton behavior. Keep platform/debug/render-pass input under clearly named application-control functions rather than moving it into Game.
- Make the overlay timer cpp-local and remove its implementation-detail unit test at `GameFrameworkTests.cpp:542-552` in this same stage. Remove the font forwarding helper.
- Update `GameApplicationTests.vcxproj`, which directly compiles application sources rather than linking the WindowedApp: add `GameApplication.cpp`, keep references only to GameFramework/Graphics/CommonUtilities/Logger and external libraries, and do not add a Game executable project reference.

Verification: existing runtime scenarios behave identically; hidden/visible startup, minimized skip/resize, input once per frame, overlay, debug camera, audio, D3D diagnostics, and exception cleanup still pass.

### Stage 2 — Replace ComponentRegistry with the concrete builder

Files: `Scenes/ComponentRegistry.*` -> `Scenes/WorldFromSceneData.*`, Actor access, framework tests, project/filter files.

- Introduce free `BuildWorldFromSceneData` with the existing closed visitor.
- Preserve candidate construction, diagnostics, active-camera validation, and missing-asset behavior.
- Remove the runtime's registry member.

Verification: every SceneData variant builds the same component; malformed actor/component rejects the candidate; missing mesh/material keeps its existing skip/fallback contract; camera rules remain.

### Stage 3 — Atomically make the runtime concrete and simplify scene loading

Files: `Game.h/.cpp`, `GameApplication.*`, `SceneData.h`, remove `IGame.h` and `GameContext.*`, remove `GameScene.*`, add `GameSceneLoading.h/.cpp`, Main, runtime tests, PublicGameplayConsumer target/files, project/filter/Premake entries.

- Change Main to construct `Game` and `GameApplication::Config`, construct GameApplication, and call `application.Run(game)`.
- Remove Game inheritance/virtual dispatch, `IGame`, `GameContext`, SceneSource, SceneLoadContext, the Main lambda, and the stateful GameScene wrapper together. Do not introduce a transitional interface, callback, or engine-to-application include.
- Add free game-local `LoadAndPrepareGameSceneData`; move SceneId/path/display mapping into application code; keep SceneData, importer, and `BuildWorldFromSceneData` boundaries.
- Fold World, optional current/pending SceneId, client size, content root, and quit state directly into GameApplication.
- Add only `RequestSceneLoad` and `ReloadCurrentScene` to the Game-facing Runtime API. Actual Game input callbacks capture Runtime and remove those captures during `Game::Shutdown()`; components receive neither operation.
- Bind Escape and register its listener in `InitializeInputAndApplicationControls`; pressing it sets GameApplication's internal quit flag. `WM_QUIT` sets the same flag. Do not expose quit through Game, GameContext, or a public callback merely to make Escape work.
- Apply the final transition policy here, once: consume one captured concrete id at the single-threaded boundary, preserve a newer pending request, do not auto-retry a failed later candidate, and make initial pre-commit/post-commit failures fatal.
- Make `Game::Shutdown()` independent of World. Unify normal/exception cleanup so a shutdown throw still clears World/render references/services and is rethrown, while a secondary cleanup throw preserves the earlier primary failure.
- Clear World, then Game and Runtime listeners/mouse, then snapshot/widget/font references, then call existing `ServiceLocator::KillServices` before Runtime/window destruction.
- Delete `PublicGameplayConsumer.cpp` and `PublicGameplayConsumer.vcxproj` (and any solution entry dedicated to that hypothetical consumer). Update `RunPublicHeaderIsolation.ps1` to remove the deleted `Runtime` folder from its folder list; it continues to compile the GameFramework headers that actually remain.
- Confirm that removing GameScene's MeshLibrary member removes no required initialization side effect; retain MeshLibrary for the concrete skeletal demo.

`GameApplicationTests.vcxproj` may continue compiling the needed application implementation directly if that remains the simplest test target. Replace obsolete source entries after moves and keep only the lower-library references the concrete code needs. Prefer tests that run the actual GameApplication with real Game behavior, real scene requests, and staged Content. Delete or simplify fixtures that exist only to implement the former `IGame`/`GameContext`/`SceneSource` contract.

Verification: entry/lifecycle order; initial scene and one later transition; reload; Escape and window close both exit through GameApplication's quit state; later failure preserves old World, reports once, and leaves no implicit retry; request queued during activation survives; listener removal; BeginPlay/EndPlay counts; representative initialize, activation/update, and shutdown failures where focused injection is materially justified; actual Blockout/chest imports and material fallback; no AssetHandling edits.

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
| A generic or elaborate test harness recreates removed abstractions | Architecture review: GameApplication calls concrete Game; obsolete generic-runtime fixtures are removed or simplified; any test-only control is tiny, event-specific, materially necessary, and compiled only in the test target. |
| Escape/quit ownership leaks back into Game or a generic Context | Verify Escape is bound and handled by GameApplication, `WM_QUIT` reaches the same internal flag, Game exposes no quit API, and Runtime removes its listener during cleanup. |
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
- GameApplicationTests scenarios covering sample/chest content, text overlay, Escape quit, camera/render-pass controls, invalid initial load, and representative lifecycle failures, using actual Game/runtime behavior wherever possible and only minimal event-specific test controls where materially needed;
- actual staged Content run for imported scenes and material fallback;
- public/header smoke for the APIs that remain intentionally supported;
- D3D diagnostic queue inspection where available.

## 12. Final simplicity and readability audit checklist

### Entry and ownership

- [ ] Starting at Main, a reader reaches concrete Game and concrete GameApplication without an interface or forwarding lambda.
- [ ] GameApplication visibly owns the loop, current World, pending/current scene state, window, and per-run platform/render state.
- [ ] ServiceLocator visibly owns InputMapper, AudioManager, and AssetRegistry.
- [ ] GraphicsEngine is clearly documented as a process singleton coordinated, not owned, by GameApplication.
- [ ] World -> Actor -> Component remains the sole live gameplay ownership chain.

### Runtime flow

- [ ] Startup order is readable in one function or one short sequence of meaningfully named phase methods.
- [ ] Frame order is readable without jumping through Context or callback wrappers.
- [ ] Escape and `WM_QUIT` both set GameApplication's internal quit state; Game and Components expose no quit API.
- [ ] Shutdown order shows Game, World, listener, service, and window cleanup directly.
- [ ] Exception cleanup attempts Game shutdown at most once and preserves the original failure.

### Scene flow

- [ ] There is one optional pending request and one explicit optional/current scene value.
- [ ] A boundary consumes one captured request before loading.
- [ ] New requests remain queued for the next boundary; last write wins.
- [ ] Recoverable candidate failure reports once and never auto-retries.
- [ ] Initial pre-commit and post-commit failures are explicitly fatal.
- [ ] SceneId/file mapping is game-local.
- [ ] The path is direct: game-local load/prepare -> importer -> SceneData -> `BuildWorldFromSceneData` -> candidate -> commit.
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
- [ ] Tests favor the actual Game/runtime path; obsolete generic-runtime tests are removed or simplified instead of being preserved through a replacement interface, Context, callback system, or scripted harness.
- [ ] Any test-only control is tiny, materially necessary, named for the concrete runtime event it controls, and absent from production builds.
- [ ] Touched functions, classes, fields, and files have purpose-revealing names; longer explicit names are preferred over hidden or ambiguous responsibility, without unrelated rename churn.
- [ ] PublicGameplayConsumer source/project are deleted; public-header isolation covers only supported GameFramework headers.
- [ ] Project files, filters, Premake, and authoritative docs match the final file layout.

## 13. Independent audit and revision record

The completed draft was independently audited from four perspectives and revised before delivery:

- **Simplicity / unnecessary abstraction:** removed the proposed one-line `Game::Run`, chose direct `application.Run(game)`, made scene loading a free game-local function, and excluded optional importer/ServiceLocator cleanup.
- **Readability / newcomer tracing:** specified the two-operation Game-facing scene API, the lifetime of Runtime-capturing input callbacks, nullable pre-initial-World state, concrete SceneId consumption, and deletion of PublicGameplayConsumer.
- **Ownership / lifecycle correctness:** removed World from Game shutdown, made World cleanup a Runtime responsibility, added render-reference release before AssetRegistry deletion, and specified best-effort cleanup with primary-exception preservation.
- **Implementation feasibility / scope:** moved the runtime mechanically before the atomic contract removal, made exact helper/file placement advisory, preferred concrete behavior tests over preservation of generic fixtures, bounded any materially necessary test controls, clarified naming, preserved header/project direction, and documented timer-test removal plus baseline handling of external AssetHandling API drift.

The refactor is complete only when a new programmer can answer, from Main and GameApplication alone, where the program starts, who owns the loop and World, how a scene is loaded, how Game participates, where input/audio/assets come from, and who shuts every major system down.
