# Building games on GameFramework

GameFramework owns the application runtime. A game implements `IGame` and passes it
and `GameApplication::Config` to `GameApplication::Run`. ModelViewer is the first
consumer: its scene, controls, materials and mesh catalog remain in the game project.
No game code owns a worker, mutex, snapshot queue, command list or render loop.

## Onboarding reading order

Start with the game-facing files; engine threading details are optional for gameplay work.

1. `Source/GameFramework/Runtime/IGame.h`: lifecycle, phase order and the role of session hooks.
2. `Source/GameFramework/Runtime/GameContext.h` and `Input/GameInput.h`: available services, borrowed
   references, and the difference between held keys and one-shot presses.
3. `Source/Application/ModelViewer/Main.cpp` and `ModelViewer.cpp`: selecting the game,
   startup configuration, scene construction, global input and shutdown.
4. `ModelViewerScene.cpp`: composing actors from engine features and game behaviors,
   loading shared assets, and wiring cross-actor references during initialization.
5. `ModelViewerComponents.h/.cpp`: small FixedUpdate, Update and LateUpdate examples.
6. `Source/GameFramework/Components/Component.h`, `World/Actor.h` and `World/World.h`: attachment, ownership,
   activation, tick ordering and the limits of immediate collection mutation.

For engine work, continue with `Runtime/GameApplication.cpp` (input/snapshot handoff and
shutdown) and `Runtime/Internal/GameLoop.h` (fixed-step accumulation and input consumption). Comments
at each boundary explain why it exists, not just what individual statements do.

The current scene builder and asset catalog are examples to replace with authored
scene data. Comments mark the planned JSON/factory boundary explicitly; a JSON
scene importer, scene transitions and stable entity handles are not implemented yet.

## Source layout

GameFramework is grouped into `Runtime` (with engine-only `Internal` scheduling),
`Input`, `World`, `Components`, and `Diagnostics`. Headers and implementations live
together, and Visual Studio filters match the physical folders. See the
[folder guide](../Source/GameFramework/README.md) for placement rules and future
scene/asset subsystem locations.

## Project boundary

| Project | Owns |
| --- | --- |
| Game project (currently ModelViewer) | Entry point/configuration, IGame implementation, rules, scenes, player controls and game content |
| GameFramework | GameApplication, callback lifecycle, GameContext, input delivery, world/actors/components, frame scheduling and rendering handoff |
| GraphicsEngine | Rendering, GPU resources, shaders and render workers |
| Utilities | Internal scheduling primitives and shared helpers |

`Runtime/GameApplication.h`, `Runtime/IGame.h`, `Runtime/GameContext.h` and `Input/GameInput.h` are the initial
application-facing API. `Runtime/Internal/GameLoop.h` is an engine implementation detail; games do
not include it. The host implementation is hidden in `Runtime/GameApplication.cpp`.
The existing GraphicsEngine/World coupling is unchanged; removing that dependency
cycle belongs to the later render-extraction work below.

## A new game

```cpp
#include "GameFramework/Runtime/GameApplication.h"
#include "GameFramework/Runtime/GameContext.h"
#include "GameFramework/Runtime/IGame.h"
#include "GameFramework/Components/CameraComponent.h"

class MyGame final : public IGame
{
    Actor* player = nullptr;
public:
    void Initialize(GameContext& game) override
    {
        player = game.GetWorld().CreateActor("Player");
        auto* camera = game.GetWorld().CreateActor("Camera");
        camera->AddComponent<CameraComponent>("Camera", 90.0f, 1.0f,
                                              50000.0f, game.GetClientSize());
        game.SetActiveCamera(camera);
    }
    void FixedUpdate(GameContext& game, float dt) override
    {
        // Fixed-rate simulation, e.g. movement/physics rules.
        if (game.GetInput().IsKeyDown(Keys::W))
            player->SetPosition(player->GetTransform().GetPosition()
                              + CommonUtilities::Vector3f(0, 0, 100 * dt));
    }
    void Update(GameContext& game, float dt) override
    {
        // Input-driven actions, UI and other frame-time behavior.
        if (game.GetInput().IsKeyPressed(Keys::ESCAPE)) game.RequestQuit();
    }
    void LateUpdate(GameContext& game, float dt) override
    {
        // Camera follow and adjustments after component animation/update.
    }
};

// Inside the game's platform entry point:
// MyGame game;
// GameApplication::Config config;
// config.ContentRoot = absoluteContentDirectory;
// config.Title = L"My Game";
// return GameApplication{}.Run(game, config);
```

The content directory must contain the engine's `Shaders` directory. Link the same
engine libraries as ModelViewer. Use a separate game project/content directory for
each game; do not put title-specific rules or paths in GameFramework.

## Callback contract

1. `Initialize(context)` runs once, after the window and graphics exist and before
   rendering or gameplay work starts. Load the initial assets and select a camera.
2. Each gameplay frame runs zero to five `FixedUpdate(context, fixedDt)` calls,
   each followed by actor/component FixedUpdate, then one `Update(context, dt)`.
3. The engine calls `World::Update(dt)` once. Existing actor/component Update and
   LateUpdate hooks run here, including skeletal animation.
4. `IGame::LateUpdate(context, dt)` runs after all component work. The engine then
   copies the finished world into a render snapshot.
5. On exit, the host joins gameplay work and calls `Shutdown(context)` once. This
   also happens if Initialize partially fails or a later callback throws. Shutdown
   must tolerate partial initialization. The original exception is preserved if
   error cleanup also fails. Context/world remain alive during Shutdown.

All gameplay callbacks are serialized. Ordinary game code mutates actors directly
inside callbacks; it needs no locks or knowledge of render workers. Initialize and
Shutdown currently run on the calling thread and frame callbacks on the gameplay
worker (or the calling thread in synchronous mode). Thus serialized does not imply
a permanent OS thread identity. Do not start background tasks that retain context,
world, actor or input references. Those references are invalid after Run returns.
Do not call World::Update yourself; that would update components twice.

Update follows the host's elapsed frame time, independently of the fixed timestep.
When gameplay is slower than presentation, pending frames are combined into one
update with accumulated time and input. Update is not guaranteed once per displayed
frame. Elapsed time is capped at 250 ms; fixed catch-up is capped at five steps, with
excess whole steps discarded and the fractional remainder retained. This intentionally
slows simulation under overload; it is not a deterministic replay/network clock.
The default fixed timestep is 1/60 second. No render interpolation is implemented yet.

Input is a stable value during a callback. Held keys use the latest platform sample;
pressed keys are ORed and mouse deltas summed when frames are combined. Update and
LateUpdate see the same frame input. FixedUpdate has its own retained edges/deltas,
so an edge arriving on a frame with zero fixed steps reaches the next fixed step,
then clears before catch-up steps. Handle each action in either Update or FixedUpdate
to avoid intentionally observing it in both domains. Multiple presses of the same
key within one combined sample collapse to one edge. Releases/text/gamepads are not
yet exposed. Optional RMB mouse-look capture is configured per game.

`Config::ThreadedUpdate = false` (or `AGP_DISABLE_THREADED_UPDATE`) uses the same phase
and snapshot path for debugging. Render diagnostics are opt-in: F6 cycles passes and
P logs statistics. The game retains its own P light diagnostics.

## Current limits and planned increments

The runtime boundary is implemented, but the existing asset and entity APIs are not
yet a complete general-purpose game SDK. Build out these services behind GameContext:

1. **Content service:** move reusable mesh importing/caching out of ModelViewer's
   MeshLibrary, expose typed asset handles, and keep game catalogs/material paths in
   the game. GPU uploads and runtime material edits should be queued internally.
   Currently load assets/materials in Initialize and keep shared render assets stable
   during play; changing actor transforms, animation and lights is supported.
2. **Entity lifecycle:** stable actor/component handles, deferred spawn/removal during
   component iteration, scene transitions and BeginPlay/EndPlay. Creation from the
   top-level game callbacks is safe; modifying actor/component collections from a
   component's own iteration still requires this work. No destruction API is added
   by this migration. Keep the selected camera alive for the session.
3. **Simulation services:** physics integration inside fixed phases, explicit tick ordering and pause/time scale. Components now support FixedUpdate as well as Update/LateUpdate.
4. **Render extraction:** move world-to-snapshot conversion behind an engine adapter,
   replace mutable shared material state with versioned data, and add interpolation.
5. **Platform/services:** input actions/rebinding, release/text/gamepad input, resize,
   audio, save data and background loading with completion delivered in game callbacks.

These increments should extend the reusable engine API, with ModelViewer and a second
small game exercising each addition. Game code should never receive scheduler queues
or rendering-thread callbacks as a shortcut.

## Validation

`Tests/EngineOptimisations` covers callback ordering, no-fixed-step frames, fixed
catch-up, retained/one-shot input edges, mouse accumulation, invalid timesteps and
callback exception propagation alongside the existing snapshot/worker tests.
Build ModelViewer Debug x64 and run the CPU test executable. Manual acceptance should
exercise scene rendering, RMB flight, animation keys, light controls, F6, P, close,
and synchronous mode. Automated tests do not validate visual output or GPU lifetime.

## ModelViewer as a working game

- `ModelViewer.cpp`: session Initialize/Update/Shutdown; Escape requests exit.
- `ModelViewerScene.cpp`: creates the original floor, chests, color checker,
  animated character, camera and lights, and attaches game behavior components.
  Materials load from the built `Assets/Shaders` content, not the source tree.
- `ModelViewerComponents.cpp`: actor behavior examples. SpinComponent rotates the
  opaque chest in FixedUpdate (R pauses/resumes); AnimationControlsComponent reads
  numpad 0-3 in Update; CameraControlsComponent moves in LateUpdate; the scene-controls
  actor processes light shortcuts in LateUpdate after the camera's final pose.

The world automatically ticks enabled components on active actors, in insertion
order. All Update calls precede all LateUpdate calls. Animation controls are attached
before the skeletal mesh so playback changes take effect in the same update.
Game-owned components receive GameContext in their constructor, e.g.:

```cpp
chest->AddComponent<SpinComponent>("Spin", context);
```

Run ModelViewer Debug x64 normally. The opaque chest rotates automatically, R toggles
rotation, RMB enables mouse look, WASD/Space/Ctrl move, numpad 0-3 select animation,
7/8/9 toggle lights, Shift+7/8/9 position/aim lights using the camera, F6 cycles render
passes, P prints diagnostics, and Escape exits. Floor and transparent chests remain
static. No manual component ticking belongs in the game class.

`Tests/GameFramework/GameFrameworkTests.vcxproj` verifies real world/component
phase ordering and inactive/disabled behavior. Build it with Configuration=Debug
and Platform=x64, then run `Bin/Tests/Debug/GameFrameworkTests.exe`.
