# Architecture Audit — `text-rendering-current`

> Historical snapshot: input architecture descriptions below are superseded by [Input restoration](../InputRestorationPlan.md).

No source files were modified during this audit. The working tree, current build manifests, generated Visual Studio projects, relevant history, and a Debug x64 validation build were inspected. The build completed successfully, but exposed a duplicate-shader compilation collision described below.

The pre-existing changes in `GameApplication.cpp`, `InputSystem.cpp`, `CommonUtilities.vcxproj`, and the tests were preserved.

## Current Runtime Architecture

```text
wWinMain
  ├─ constructs Game                         project-specific IGame
  ├─ constructs GameScene                    scene name → JSON/import/assets
  └─ GameApplication::Run
       └─ GameApplication::Impl
            ├─ owns Window / platform input backends
            ├─ owns GameContext
            │    ├─ owns InputSystem
            │    └─ owns current World
            │         └─ owns Actors
            │              ├─ Actor Transform
            │              └─ owns Components
            ├─ owns ComponentRegistry
            ├─ owns render snapshot/command list
            ├─ initializes singleton GraphicsEngine
            ├─ initializes singleton AssetRegistry
            ├─ initializes singleton AudioManager
            └─ publishes those services through ServiceLocator

Scene path:
Game::Initialize
  → GameContext::LoadScene("ChestMaterials")
  → GameScene::Load
  → UnrealSceneImporter
  → Perforce-shaped DTOs
  → intermediate Unreal DTOs
  → SceneData
  → GameScene::PrepareAssets
  → ComponentRegistry::CreateWorld
  → World / Actors / Components
  → Game::ConfigureWorld
  → World::BeginPlay

Render path:
World
  → WorldRenderer::Build
  → GraphicsEngine::RenderSceneSnapshot
  → GraphicsEngine::RenderSnapshot
  → GraphicsCommandList
  → RenderHardwareInterface / DirectX 11
```

The central entry points are `Source/Application/Game/Main.cpp` and `Source/Engine/GameFramework/Runtime/GameApplication.cpp`.

This is fundamentally the simplified MVP architecture. The old `Runtime/Internal`, handle/reference system, scene service, multi-stage world states, and public/private integration layers are not present in the current tree. Git history confirms they were removed in the MVP transition.

## Startup Flow

1. Windows enters `wWinMain`.
2. `Main.cpp` finds `Game.exe` and computes:

   ```text
   <exe directory>/../../Content
   ```

   For `Bin/Debug/Game.exe`, this resolves to the repository's `Content` directory.
3. Stack-owned `Game` and `GameScene` objects are created.
4. Temporary `GameApplication` creates its private `Impl`.
5. `Impl` creates the Win32 window.
6. `ContentRoot` is canonicalized.
7. `GraphicsEngine::Get()` initializes using `ContentRoot/Shaders`.
8. `AssetRegistry::Get()` recursively indexes `Content`.
9. The render-diagnostics font and `TextWidget` are created if enabled.
10. `AudioManager::GetInstance()` is initialized.
11. Input, audio, and assets are registered in `ServiceLocator`.
12. `InputHandler` is attached to the window and default action bindings are installed.
13. Host-level input subscriptions are installed.
14. `Game::Initialize` installs game subscriptions, requests `ChestMaterials`, and starts music.
15. The pending scene is loaded immediately.
16. `World::BeginPlay` runs.
17. The window is shown and the main loop begins.

## Exact Frame/Update Flow

There are no literal `BeginFrame` or `EndFrame` functions. Their responsibilities are spread across the host and renderer.

1. **Win32 message processing — `GameApplication`**
   - `PeekMessageW`
   - `InputHandler::UpdateEvents`
   - `TranslateMessage` / `DispatchMessageW`
   - `WM_QUIT` becomes `GameContext::RequestQuit`.

2. **Pending scene replacement — `GameApplication::LoadPendingScene`**
   - Scene requests raised during the previous frame are handled here.
   - Loading time is excluded from the next movement delta.

3. **Frame timing — `CommonUtilities::Timer`**
   - Delta is clamped to `[0, 0.25]`.

4. **Physical input update — `GameApplication`**
   - `InputHandler::UpdateInput`
   - `CaptureInputFrame`
   - Keyboard state is the OR of `InputHandler::IsKeyDown` and `GetAsyncKeyState`.
   - Mouse delta is calculated by cursor recentering.
   - `XInputHandler::UpdateInput` samples the gamepad.

5. **Action/event input — `InputSystem`**
   - The physical frame becomes named actions.
   - `Started`, `Ongoing`, and `Ended` callbacks run synchronously.
   - Game, host, and component subscribers may act immediately.
   - A scene request made here is not applied until the next loop iteration.

6. **Project-level update — `Game::Update`**
   - Presently empty.

7. **Gameplay/component update — `World::Update`**
   - Removes previously destroyed actors/components.
   - Freezes the frame's component list.
   - Calls `BeginPlay` for newly attached components.
   - Updates enabled components on active actors, in actor/component insertion order.
   - Skeletal animation and gameplay behaviours run here.

8. **Audio update — `AudioManager::Update`**

9. **Render extraction — `WorldRenderer::Build`**
   - Clears the prior snapshot.
   - Synchronizes the active camera.
   - Copies camera, light, mesh, material, transform, and skeletal-pose data.
   - `GraphicsEngine::FinalizeRenderSnapshot` performs culling, pass classification, and sorting.

10. **Overlay injection — `GameApplication`**
    - The render-pass notification `TextWidget` is appended after world extraction.

11. **Render command setup — `GameApplication`**
    - `GraphicsCommandList::ResetCommandList`.

12. **Renderer — `GraphicsEngine::RenderSnapshot`**
    - Prepare lazy render resources.
    - Build shadow jobs.
    - Record/join/execute shadow work.
    - Prepare scene state.
    - GBuffer pass.
    - SSAO.
    - Deferred lighting/composite.
    - Optional debug view.
    - Transparent geometry.
    - Screen text.

13. **Frame completion — `GameApplication`**
    - `FinishCommandList`
    - `ExecuteCommandList`
    - `Present`

If command-list finishing fails, that frame is not executed or presented.

## Scene Replacement

`GameContext::LoadScene` only stores the latest requested name. At the next frame boundary:

```text
SceneSource callback
  → GameScene::Load
  → import and asset preparation
  → ComponentRegistry creates candidate World
  → Game::ConfigureWorld(candidate)
  → clear old World
  → install candidate
  → reset input action state
  → reset debug-camera state
  → select imported camera or create debug camera
  → candidate.BeginPlay()
  → Game::OnSceneLoaded()
```

Construction failure keeps the current world alive. Initial-scene failure is fatal.

## Shutdown Flow

Normal shutdown is:

```text
stop accepting scene requests
  → Game::Shutdown
       → stop music
       → release game input subscriptions
       → clear active camera
  → World::Clear
       → Actor destruction
       → reverse-order Component::EndPlay
  → ServiceLocator::Clear
  → AudioManager::Shutdown
  → AssetRegistry::Clear
  → GameApplication::Impl destructor destroys window
  → GraphicsEngine singleton/RHI die later during static destruction
```

There is no explicit `GraphicsEngine::Shutdown`. That is a lifecycle gap, particularly if application restart, multiple runs in one process, or deterministic teardown ever matters.

## Classification Table

| System/File | Classification | Evidence | Recommendation |
|---|---|---|---|
| `Main.cpp` | Current | Only executable entry; constructs `Game`, `GameScene`, and calls `Run` | Keep, but centralize runtime-path resolution |
| `GameApplication` | Current, overburdened | Owns window, loop, platform input, service bootstrapping, debug UI, scene replacement, rendering | Keep the host; extract small platform/content/service helpers incrementally |
| `IGame`, `Game`, `GameContext` | Current | Actual startup and frame callbacks | Keep; expand `GameContext` with direct audio/assets access |
| `World`, `Actor`, `Component` | Current | Sole runtime object hierarchy and update path | Keep |
| `SceneComponent` | Current | Sole component-local transform implementation | Keep |
| `ComponentRegistry` | Current | Sole `SceneData → World` builder | Keep; rename if desired because it is now a closed builder, not a registry |
| `WorldRenderer` | Current | Sole world-to-render-snapshot adapter | Keep |
| `GraphicsEngine` / RHI | Current | Sole rendering path | Keep; add explicit lifecycle and hide singleton access from higher layers |
| `InputSystem` | Current | Gameplay consumes named action events | Keep |
| `InputHandler`, `XInputHandler` | Current backend | Used only by the host to populate `InputDeviceFrame` | Keep as private platform backends |
| `GetAsyncKeyState` alongside `InputHandler` | Duplicate, both active | Every key is read from both sources and ORed | Choose one keyboard source; retain polling only for explicitly documented edge cases |
| `ServiceLocator` | Transitional | Production uses it for audio; input/assets entries have no production consumers | Migrate audio/assets into `GameContext`, then remove |
| `AudioManager` singleton | Current functionality, legacy ownership | Manually allocated singleton, also exposed through locator | Make host-owned and expose a narrow context/service interface |
| `AssetRegistry` | Current functionality, legacy ownership | Global singleton used throughout loading | Make host-owned; pass/reference it through `GameContext` and scene context |
| `MeshLibrary` | Transitional duplicate | Second cache/loader owned by `GameScene`; registry delegates mesh loading to it | Move the FBX loader adapter behind the asset registry; remove the second public cache |
| `MeshLibrary::LoadSceneMesh` | Dead | No call sites | Delete |
| `MeshLibrary::LoadFBXAnimation` | Dormant | No production or test call sites | Delete if animation import is out of current scope; otherwise migrate behind assets |
| `SceneData` | Current | Runtime's typed scene boundary | Keep |
| `PerforceSceneStructs` | Transitional compatibility | Active parser DTO matching Perforce shape | Keep until parser/export contract is stabilized |
| `UnrealSceneStructs` intermediate DTO | Duplicate/transitional | Perforce DTO is converted into a second source DTO before `SceneData` | Collapse one conversion layer after importer tests cover the exporter contract |
| Placeholder scene components | Transitional | Imported box/sphere/capsule/spring-arm data survives but has no runtime behaviour | Keep until physics/gameplay ownership is decided |
| `CameraControlsComponent` | Dormant duplicate | Used by tests, never attached by current game | Decide whether it remains a sample controller; otherwise remove with its actions/tests |
| `DebugCameraController` | Current | Current `ChestMaterials` scene has no active camera; host creates this controller | Keep |
| Animation/light control components | Dormant | Compiled and tested but never attached by current executable | Treat as samples or remove; do not leave as ambiguous production systems |
| `Source/Engine/GraphicsEngine/Shaders` | Current source tree | Premake copies from here; text shaders live here | Make this the sole engine-shader source of truth |
| `Content/Shaders` | Duplicate but runtime-active | Runtime loads from here; Debug build overwrites engine subtrees here; Git tracks them | Keep game `.mat/.hlsli` content; stop tracking copied engine shader subtrees |
| `Source/Engine/GraphicsEngine/Content/Shaders` | Legacy duplicate, still built | Byte-identical old tree; generated project compiles 13 shaders from it | Remove after project regeneration |
| `TemporaryShaders/*.h` | Generated/dead | Generated headers have no C++ consumers and are tracked | Remove and ignore the current path |
| `TextWidget`, `Font`, text shaders | Current rendering path | Loaded through `AssetRegistry`, appended to the current snapshot, rendered in the final overlay pass | Keep |
| Text gameplay API | Missing/transitional | Only host diagnostics can extract `FontAsset` through friendship and submit text | Add a small framework-facing UI/text service or component before gameplay text expands |
| `Game.sln` | Duplicate | Byte-for-byte identical to `AGP.sln` | Delete one; keep `AGP.sln` |
| Checked-in `.vcxproj` plus premake | Duplicate build authority | Both are versioned and currently drift-prone | Declare premake authoritative, regenerate in CI, or explicitly declare projects authoritative |
| `AGPCleaner.bat` | Legacy/broken | Copies `Assets` and removed `Game/Materials`, but not `Content`; packaged executable computes a nonexistent content root | Replace after deciding final deployment layout |
| `GameOrientation.h` | Dead | No references | Delete |
| `ModelViewer.rc/.ico`, `small.ico` | Legacy artifacts | Viewer sources are explicitly excluded; resources are unused | Delete |
| README/MVP/import docs | Transitional/stale | Several statements contradict current code, importer, assets, and ServiceLocator | Update after architecture decisions |

## Shader and Content Findings

The runtime shader root is:

```text
Bin/Debug/Game.exe
  → parent = Bin/Debug
  → ../.. = repository root
  → Content
  → Content/Shaders
```

`GraphicsEngine` runtime-compiles shaders from that path, including the new text shaders.

There are currently three engine-shader trees:

1. `Source/Engine/GraphicsEngine/Shaders` — intended authoring source.
2. `Content/Shaders` — runtime copy plus game material files.
3. `Source/Engine/GraphicsEngine/Content/Shaders` — imported duplicate.

All shared files across the three trees are currently byte-identical. The third tree lacks the new text shaders, showing that it is stale rather than authoritative.

The copy happens at **build time**, through the GraphicsEngine custom build step:

- Debug copies `Source/Engine/GraphicsEngine/Shaders` into repository `Content/Shaders`.
- Release/Retail copy shaders into `Bin/<Configuration>/Shaders`.

This policy is inconsistent with runtime resolution:

- Debug uses the copied destination.
- Release still computes repository `Content/Shaders`, not `Bin/Release/Shaders`.
- The release copy destination is therefore ignored by the executable.
- A checkout can mask this because `Content/Shaders` is tracked.
- The packaging script does not copy `Content`, so its produced application layout cannot satisfy `Main.cpp`'s content-root computation.

Both `Content/Shaders` and the two source trees are version-controlled. The ignore file still references obsolete paths such as `/Assets/Shaders/` and `/Source/Graphics/GraphicsEngine/TemporaryShaders/`.

The generated GraphicsEngine project actively compiles both source trees. During the validation build, FXC compiled the duplicate shader set twice and both copies targeted the same generated header filenames. It emitted:

```text
failed writing ...\PrecompiledShaders\RenderPassDebug_PS.h
```

The overall build still reported success. Those generated headers are not consumed by C++; the engine recompiles HLSL at runtime. This is needless work and a nondeterministic output collision.

## Text Rendering Assessment

The text implementation uses the current renderer:

```text
AssetRegistry::ResolveFont
  → Font + atlas texture
  → TextWidget CPU geometry
  → RenderSceneSnapshot::ScreenTextItems
  → GraphicsEngine::RenderScreenText
  → TextOverlay shaders
```

It does not depend on the old GameFramework lifecycle, old render bridge, or removed worker/snapshot architecture.

The architectural issue is at the API boundary:

- `TextWidget` is a GraphicsEngine type.
- `GameApplication` reaches through opaque `FontAsset` using friendship.
- The widget is manually inserted into the renderer snapshot.
- There is no gameplay-facing text/UI component or context service.

That is acceptable for a small host diagnostic overlay, but it should not become the gameplay text API. Keep the rendering implementation and later wrap submission/font resolution in a small framework-facing UI service.

## Input Assessment

The active high-level system is `InputSystem`. There is no current `InputMapper` or `GameInput` implementation.

The current layers are:

```text
Win32 messages → InputHandler ┐
GetAsyncKeyState polling      ├→ InputDeviceFrame → InputSystem → action events
XInputHandler                 ┘
```

`InputHandler` and `XInputHandler` are required backends, not legacy competitors. The true duplication is keyboard sampling through both `InputHandler` and `GetAsyncKeyState`.

Several default actions are also installed even though their only consumers are dormant demo components. That makes the default engine action map project-specific and larger than the actual runtime behaviour.

## Main Architectural Problems

Ordered approximately by impact:

1. **Shader/content/build layout has three sources of truth.**  
   It causes duplicate compilation, generated-output collisions, release/runtime disagreement, and broken packaging.

2. **Service ownership is only superficially simplified.**  
   `GameContext` cleanly owns world/input, but graphics, assets, and audio remain singletons; `ServiceLocator` adds another global doorway over two of them.

3. **Asset loading is split across two owners.**  
   `AssetRegistry` is the public cache, while game-owned `MeshLibrary` owns FBX initialization, another cache, primitive creation, and raw renderer resources.

4. **The importer has three active representations.**  
   JSON becomes Perforce DTOs, then another Unreal DTO set, then runtime `SceneData`. The boundary is useful; both intermediate DTO sets are not.

5. **Build metadata has competing authorities.**  
   Premake, checked-in project files, two solutions, manually maintained tests, stale copied shaders, and obsolete ignore rules disagree.

6. **`GameApplication` is a composition root plus platform layer plus debug UI plus render driver.**  
   Gameplay is shielded from it, which is good, but service and content setup changes all converge in one file.

7. **Camera/control sample code is ambiguous.**  
   The debug camera is active. The very similar game camera controller and several other controls are compiled and tested but not used by the current executable.

8. **Text rendering is current internally but has no appropriate gameplay boundary.**

9. **Lifecycle is inconsistent.**  
   World and audio have explicit shutdown; GraphicsEngine and RHI rely on process-static destruction.

10. **Documentation and packaging still describe removed layouts.**

## Recommended Target Shape

No rewrite is required. The existing MVP core is a good base:

```text
GameApplication
  ├─ owns GraphicsEngine
  ├─ owns AssetRegistry
  ├─ owns AudioManager
  └─ owns GameContext
       ├─ World
       ├─ Input
       ├─ Assets view
       ├─ Audio view
       └─ UI/Text view

MyGame::Start(GameContext&)
MyGame::Update(float)
```

`IGame::Initialize(GameContext&)` is already functionally close to the desired `Start`. The highest-value work is to make the service ownership match that clean surface, not replace World/Actor/Component.

## Things That Are Safe to Delete

After removing their project entries or regenerating projects:

- `Source/Engine/GraphicsEngine/Content/Shaders/`
- `Source/Engine/GraphicsEngine/TemporaryShaders/*.h`
- Checked-in source-local `Lib/Debug` outputs
- `Game.sln`, retaining identical `AGP.sln`
- `GameOrientation.h`
- `ModelViewer.rc`
- `ModelViewer.ico`
- `small.ico`
- Empty `Source/Application/Game/Materials`
- `MeshLibrary::LoadSceneMesh`
- Stale untracked `Assets/Shaders`

## Things That Need Migration Before Deletion

- `ServiceLocator`
- `AssetRegistry::Get()` singleton access
- `AudioManager::GetInstance()`
- `GraphicsEngine::Get()`
- `MeshLibrary` as a whole
- Copied engine shaders under `Content/Shaders`
- One of the two importer DTO layers
- Dormant camera/animation/light controller components
- Corresponding unused default input actions
- Generated project files, if premake becomes the sole authority
- `AGPCleaner.bat`
- Placeholder component/source-parent compatibility data

## Things That Should Stay

- `GameApplication` as the reusable host
- `IGame` and `GameContext`
- `World → Actor → Component` ownership
- `Transform` and `SceneComponent`
- Typed `SceneData`
- Candidate-world scene replacement
- `ComponentRegistry`'s construction role
- `WorldRenderer` snapshot adapter
- `InputSystem` and subscription model
- `InputHandler`/`XInputHandler` as hidden platform backends
- `AssetId` and opaque gameplay-facing asset handles
- Unreal JSON importer boundary
- `Source/Engine/GraphicsEngine/Shaders` as canonical shader source
- Current text renderer, font loader, and text shaders
- Renderer-owned multithreaded shadow recording

## Files and Systems to Investigate Manually

- Perforce mappings or build scripts that may still expect `Source/Engine/GraphicsEngine/Content`.
- Whether checked-in `.vcxproj` files or premake scripts are authoritative for the team.
- Whether `Content/Shaders` must be depot-managed for non-programmers, or should be generated.
- Desired packaged directory layout for `Content`, dependencies, and audio banks.
- The hard-coded audio bank path relative to the working directory.
- Whether dormant camera, animation, and light controls are intended samples or abandoned runtime features.
- Exporter semantics for component `Parent`, camera tags, and placeholder collision/spring-arm data.
- Duplicate/nested `lvl_blockout` scene export directories.
- Why tests are separate project files but absent from the premake workspace.
- Stale statements in `README.md`, `GameFrameworkMVP.md`, and `PerforceIntegrationChanges.md`.

## Phased Implementation Plan

### Phase 1: Make the Build Deterministic

- Choose `Source/Engine/GraphicsEngine/Shaders` as canonical.
- Exclude and delete `GraphicsEngine/Content/Shaders`.
- Stop generating unused shader headers, or give validation outputs unique paths.
- Fix current `.gitignore` paths.
- Add a clean-build shader-tree consistency check.

### Phase 2: Fix Runtime Content Deployment

- Define one executable-relative content layout for Debug, Release, Retail, tests, and packaged builds.
- Copy the entire required `Content` tree to that layout.
- Make `Main.cpp` resolve that location consistently.
- Replace the obsolete cleaner rules.

### Phase 3: Remove Obvious Dead Artifacts

- Delete duplicate solution/resources/generated libraries and dead methods.
- Regenerate projects.
- Update documentation in the same small change.

### Phase 4: Make Service Access Explicit

- Add `GetAssets()` and `GetAudio()` to `GameContext`.
- Migrate `Game.cpp` away from `ServiceLocator`.
- Stop publishing unused input/assets entries through the locator.
- Delete `ServiceLocator` once callers and tests are migrated.

### Phase 5: Consolidate Asset Ownership

- Move FBX loading and importer initialization behind `AssetRegistry` or an injected mesh-loader service owned by the host.
- Move primitive registration there.
- Remove `MeshLibrary`'s duplicate cache.
- Keep opaque asset handles at the gameplay boundary.

### Phase 6: Simplify Scene Importing

- Preserve the tested `JSON → SceneData` contract.
- Collapse either `PerforceSceneStructs` or the intermediate Unreal DTOs.
- Keep source-format conversion out of `World`.
- Resolve the intended treatment of parent metadata and placeholders.

### Phase 7: Trim Sample/Runtime Ambiguity

- Decide which camera/controller is the supported gameplay example.
- Move optional sample components out of engine defaults or remove them.
- Split engine input actions from game/demo bindings.

### Phase 8: Add a Gameplay-Facing Text Boundary

- Keep `TextWidget` and GPU work internal.
- Expose simple font/text creation or a screen-UI component through `GameContext`.
- Let gameplay submit text without including GraphicsEngine or touching snapshots.

### Phase 9: Finish Lifecycle Cleanup

- Give graphics, assets, and audio explicit host-owned lifetimes and shutdown order.
- Keep renderer threading entirely behind `WorldRenderer`/`GraphicsEngine`.
- Add one startup/frame/scene-reload/shutdown integration test around the final ownership model.

## Conclusion

The old complicated GameFramework runtime itself is largely gone. The remaining complexity is concentrated around the Perforce integration seams: global services, asset loading, importer DTO layers, copied shader trees, and stale build/deployment structure.

These areas can be simplified incrementally without changing the successful core:

```text
World → Actor → Component → Render Snapshot → GraphicsEngine
```
