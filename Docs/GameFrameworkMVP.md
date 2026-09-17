# GameFramework MVP

The MVP keeps the scene/object foundation and one main update loop.
GameFramework has 33 C++ files (down from 62) and roughly 2,500 lines (down from
5,500). The original implementation remains on the `game-framework` branch.

## Start here

Headers and implementations live together in Runtime, World, Components, Scenes
and Rendering. The same folders appear in Solution Explorer. Rendering is the
engine adapter; the other four folders contain the gameplay-facing headers.

Read these files in order:

1. `Source/Application/Game/Main.cpp`: create Game and its scene source, call Run.
2. `Source/Application/Game/Game.cpp`: register behavior, request a scene, handle session input.
3. `Source/GameFramework/Scenes/SceneData.h`: the entire scene-description boundary.
4. `Source/GameFramework/Scenes/ComponentRegistry.cpp`: descriptions become owned runtime objects.
5. `Source/GameFramework/World/World.h`, `Actor.h`, `Component.h`: ownership and the small API.
6. `Source/GameFramework/World/World.cpp` and `Actor.cpp`: startup, Update and destruction.
7. `Source/Application/Game/GameComponents.cpp`: camera, spin, animation and light behavior.
8. `Source/GameFramework/Runtime/GameApplication.cpp`: the actual main loop and scene replacement.
9. `Source/GameFramework/Rendering/WorldRenderer.cpp`: values copied to the existing renderer.

## Ownership and flow

GameApplication owns GameContext. GameContext owns one World. World owns Actors,
and each Actor owns its Components with `unique_ptr`. Actors have a transform;
a SceneComponent adds a local camera/mesh/light offset.

```text
Game::Initialize -> LoadScene("Game")
                         |
                     SceneData
                         |
                       World
                         |
                       Actor
                         |
     ComponentRegistry -> Component -> apply properties
                         |
                      BeginPlay
                         |
   each frame: Game::Update -> Component::Update -> renderer
                         |
                       EndPlay
```

All gameplay runs on the application thread. The loop samples input, calls the
game, updates components once, builds a snapshot, and renders it. The renderer's
existing shadow workers remain renderer details. Gameplay has no worker, mailbox,
mutex, triple-buffer queue, fixed step or late phase.

## Minimal behavior

```cpp
#include <GameFramework/Runtime/IGame.h>
#include <GameFramework/Runtime/GameContext.h>

class Move final : public Component
{
    void Update(float dt) override
    {
        auto& transform = GetOwner()->GetTransform();
        transform.SetLocalPosition(transform.GetLocalPosition()
                                   + CommonUtilities::Vector3f{0, 0, 100 * dt});
    }
};

class Example final : public IGame
{
    void Initialize(GameContext& game) override
    {
        auto* player = game.GetWorld().SpawnActor("Player");
        player->AddComponent<Move>();
        // player->GetComponent<Move>() finds the owned behavior.
        // player->Destroy() stops its updates immediately.
    }
};
```

For scene-created types, register a factory/property reader in RegisterComponents:

```cpp
registry.Register<MyComponent>("MyComponent",
    [](MyComponent& component, const SceneReader& fields)
    {
        component.Speed = fields.OptionalFloat("speed", 100.f);
    });
```

The registry header is `GameFramework/Scenes/ComponentRegistry.h`. Properties are a small
variant map. Factories construct and configure only their own component. The
registry creates every Actor and Component before the host calls BeginPlay.
Dependencies can be found by name or GetComponent in BeginPlay or Update.
No reflection, reference binding, dependency resolver or mutation guard is involved.

## Scene loading

`game.LoadScene("Level")` records a request; the host handles the latest request
at the next frame boundary. A SceneSource callback returns SceneData and can put
ready mesh/material bindings into the supplied AssetLibrary. GameScene is the
current C++ source; a future importer adapts to the same descriptions.

Construction errors include Actor/component/property context. A failed load
keeps the old World. A successful load clears it, installs the new World, then
calls BeginPlay and OnSceneLoaded. ReloadScene requests the current name.
There is no scene-status service or asynchronous loading pipeline.
Exceptions during BeginPlay or Update terminate the session and run cleanup.

## Lifetime and deliberate limits

- BeginPlay occurs once, including for inactive/disabled objects. Only enabled
  components on active Actors update, in insertion order.
- Additions made during component callbacks start at the next World update.
  Additions made by Game::Update are available to that frame's World update.
- Destroy marks an object immediately; memory is released at the next update
  boundary or Clear. EndPlay runs once for components whose BeginPlay was entered.
  EndPlay is noexcept: cleanup callbacks must not throw.
- Pointers are temporary borrows. Do not retain them after destruction or scene
  replacement. Store names and look up again, as the light controls do.
  Actor names must be unique per World; component names per Actor.
- No Actor hierarchy, component parenting, reparent modes or checked handles.
  Spatial components can still have local offsets relative to their Actor.
- No GameTime service. Update receives seconds, capped at 250 ms; zero/invalid
  deltas become zero and still produce one update.
- Scene loading is synchronous. There is no physics, pause system, streaming,
  event bus, reflection, editor integration or new importer.
- Property readers reject wrong types for fields they read. Unused fields are
  ignored; strict schema diagnostics are deferred.
- Meshes, materials, skeletal playback, cameras, lights and render passes reuse
  the existing graphics engine.

## What was removed

The entire Runtime/Internal directory; SessionState; WorldAccess, InputAccess,
TimeAccess, RegistryAccess, SceneServiceAccess, AssetAccess and RenderAccess;
ObjectRef/ObjectSlots; GameLoop; GameTime; SceneService, SceneId, SceneLoadError and
SceneDiagnostic; References; SceneBuilder; TransformOperations and ReparentMode;
the Integration include layer; separate framework logging files; and the empty
StaticMeshComponent implementation.

The old world states, Prepare/Activate/Flush stages, reference-fixup passes and
configuration mutation guards were removed from the implementation as well.
The old expanded tests and duplicated framework plans are removed from this branch.
They remain in the original branch for future reference.

## Checks

GameFrameworkTests covers ownership, Add/Get, once-only startup/cleanup, disabled
objects, runtime additions/destruction, one update per frame, delta/input behavior,
scene factories/properties, useful failures and camera cleanup. GameRuntimeTests
uses the actual Game scene to exercise rendering, invalid replacement, reload,
empty scenes, and startup/update/shutdown failures. CameraControlsTests checks
camera math, same-frame light aiming and spin pause/resume. PublicGameplayConsumer
and RunPublicHeaderIsolation verify that gameplay headers need no graphics SDK.

Build the solution and the test projects with Visual Studio v145, x64. Run:

```text
Bin/Tests/Debug/GameFrameworkTests.exe
Bin/Tests/Debug/GameRuntimeTests.exe camera-controls
Bin/Tests/Debug/GameRuntimeTests.exe sample
Bin/Tests/Debug/GameRuntimeTests.exe invalid-initial
Bin/Tests/Debug/GameRuntimeTests.exe initialize-failure
Bin/Tests/Debug/GameRuntimeTests.exe begin-failure
Bin/Tests/Debug/GameRuntimeTests.exe update-failure
Bin/Tests/Debug/GameRuntimeTests.exe component-failure
Bin/Tests/Debug/GameRuntimeTests.exe shutdown-failure
```

Verified 2026-09-17: Debug and Release x64 builds, both core suites, camera/control
checks, real-content rendering/replacement, and all six failure scenarios passed.
All 18 public headers compiled independently without graphics/platform includes.
Runtime logs are under `Intermediate/GameFrameworkMVP/Small-Debug` and
`Small-Release`. D3D debug queues reported no errors. Existing vendor/compiler
warnings remain; no manual visual comparison or performance claim is made.

After the folder reorganization, the Debug x64 solution, core/runtime test projects
and gameplay consumer rebuilt successfully. The core suite, all eight runtime
scenarios and the 18-header isolation check passed again. Runtime logs for this
check are under `Intermediate/GameFrameworkMVP/Layout-Debug`.
