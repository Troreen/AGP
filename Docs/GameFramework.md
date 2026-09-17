# Building games on GameFramework

Implement `IGame` and pass it with `GameApplication::Config` to
`GameApplication::Run`. The host owns the world, automatic component startup,
serialized callbacks, input sampling and rendering. World owns Actors; each Actor
owns its Components. The game never calls world lifecycle or tick methods.

## Core example

Use `Source/GameFramework/Public` and `CommonUtilities/include` on the include path.
The compile-tested core example is `Tests/GameFramework/PublicGameplayConsumer.cpp`.

```cpp
#include <GameFramework/IGame.h>
#include <GameFramework/GameContext.h>
#include <GameFramework/World.h>
#include <GameFramework/GameInput.h>

class Example final : public IGame
{
public:
    void Initialize(GameContext& game) override
    {
        myPlayer = game.GetWorld().SpawnActor("Player")->GetRef();
    }
    void Update(GameContext& game, float dt) override
    {
        if (auto* player = myPlayer.Get())
        {
            player->GetTransform().SetLocalPosition({0, 0, dt * 100});
        }
        if (game.GetInput().IsKeyPressed(Keys::ESCAPE))
        {
            game.RequestQuit();
        }
    }
private:
    ActorRef myPlayer;
};
```

The content root must include the existing engine shaders. Engine built-ins
register automatically under `agp.*`; custom scene types register in the optional
`RegisterComponents(ComponentRegistry&)` hook. Direct C++ `AddComponent<T>` does
not require registration. Registration errors occur before Initialize and do not
invoke Shutdown.

## Callback and lifetime contract

Initialize is entered once after host setup. Components start after initialization
and whole-batch validation. Each frame has zero or more game/component fixed
updates, game Update, component Update and LateUpdate, then game LateUpdate.
Callbacks never overlap, but can run on different OS threads. Use input only within
the current callback. Fixed and variable phases receive separate press-edge domains;
choose one domain for each toggle.

Raw pointers and world references are short borrows. Store checked refs between
callbacks; destruction and scene replacement make them unavailable. Shutdown runs
once if Initialize was entered, including partial initialization failure. It can
borrow the world, but cannot spawn, add components or request scenes. Component
EndPlay follows Shutdown, so keep game state needed by EndPlay alive until Run returns.

## Migration status

M1–M4 implement the core object model, owned scene data, registered readers and
scene-ID requests. ModelViewer installs its C++ source once at application setup;
game callbacks call GetScenes().Load(SceneId{"ModelViewer"}) or Reload(). The legacy scene factory/Configure-lambda bridge is removed. Public contains the supported definitions, Private contains engine implementation, and Integration contains the source/data boundary.

No physics, animation graph, networking, editor, new asset manager or input system
is part of this work. Actual Perforce scene integration requires verified team code
and fixtures; the existing FBX asset importer is not that scene importer.

See [implementation evidence](SimplifiedGameFrameworkImplementation.md) for build,
test and review results and [the plan](SimplifiedGameFrameworkPlan.md) for contracts.

## Objects, transforms and references

`AddComponent<T>()` generates an instance name; the named overload accepts explicit
names and constructor arguments. Duplicate/empty explicit names throw. `GetComponent<T>()`
returns the first live attached match, including pending additions; `GetComponents<T>()`
returns all. Required dependency resolution rejects multiple matches. Actor display
names can repeat; `FindActor(name)` returns null and diagnoses ambiguity, while
`FindActors(name)` returns all. Renaming does not affect stored refs.

Use `GetTransform().SetLocalPosition`, `SetLocalRotationDegrees`, `SetLocalScale`,
`SetWorldPosition` or `SetWorldMatrix`. Setters return false without changing values
when the requested pose is invalid. Copy `LocalPose` through Get/SetLocalPose;
Transform itself cannot be copied or used to assign raw parent pointers.
`SetParent(parent, ReparentMode::KeepLocal/KeepWorld)` is immediate and returns its
actual result. KeepWorld rejects singular parents or a resulting local shear.
Actors parent within one world; spatial components parent within one actor.
An admitted child cannot parent under a pending addition until its next boundary.

An optional `ResolveReferences(References&)` validates sibling requirements before
BeginPlay. Engine mutation APIs throw during resolution, and even a swallowed
mutation exception rejects the batch. Resolution must not perform external side
effects or mutate custom peer fields. Whole-batch validation precedes all BeginPlay
calls. Inactive/disabled components still begin once. Additions from BeginPlay,
fixed/update/late callbacks or ordinary EndPlay wait for the next frame boundary.

Destroy marks the attachment subtree immediately: queries and refs stop resolving,
later callbacks skip it, and memory is collected later. Child-first EndPlay pairs
only completed BeginPlay calls; throwing EndPlay is logged and remaining cleanup
continues. Use nonthrowing destructors/RAII for allocations that never began.
OnDestroy and mixed local/effective activity notifications have been removed.

## Registered scenes

Include `<GameFramework/Registration/ComponentRegistry.h>` and
`<GameFramework/Registration/SceneReader.h>` for the registration hook. Register
stable names with a reader that configures only its own component:

```cpp
registry.Register<SpinComponent>("sample.Spin", [](SpinComponent& spin, SceneReader& fields)
{
    spin.SetDegreesPerSecond(fields.OptionalFloat("speed", 25.0f));
});
```

The engine creates every component before reading values, then resolves ID-based
bindings and optional sibling dependencies before any BeginPlay. Optional absent
fields use defaults; present wrong types, unknown fields and supplied invalid refs
are errors. BindActor/BindComponent destinations must be retained Ref fields on
the component. Factories for unusual constructors return an unattached unique_ptr;
the engine attaches it. Factories may only construct, and readers may only configure
their target. Neither can mutate other objects or request session actions.

Ordinary game callbacks include `<GameFramework/SceneService.h>` and call
`game.GetScenes().Load(SceneId{"Level1"})`. The latest request before a host boundary
wins. Reload requires a successfully loaded current ID. GetStatus, GetCurrent and
GetLastError expose the result; OnSceneLoaded follows new-world BeginPlay and
OnSceneLoadFailed reports preparation errors. Failed replacement preserves the
current world, camera, ID and refs in Debug and Release. Initial requested-load
failure returns nonzero after cleanup. Exceptions after commit end the session.
Requests during old-world teardown reject; requests from new BeginPlay wait for
the next boundary. Scene loading clears press/mouse edges and fixed accumulation;
GameTime elapsed seconds count gameplay only and continue across replacements.

Application integration adds the separate Integration include root and installs
ApplicationSetup::SceneSource when calling Run. ISceneSource returns owned SceneData
or explicit errors; valid empty data differs from failure. SourceLocation survives
into diagnostics. Actor/component IDs address authored links independently of
labels. Pose values are engine-space parent-local TRS. The source prepares immutable
mesh/material resources using the existing backend and places them in candidate-local
AssetBindings. GetAssets is ready-resource lookup, never a loader or new cache.
The importer must not return actors or drive lifecycle. See ImporterHandoff.md for
unverified Perforce requirements.

## Meshes, cameras and rendering

Mesh components bind MeshAsset and MaterialAsset values obtained from GetAssets.
SetMesh accepts a ready binding (empty clears it); SetMaterial returns false for
an invalid slot or empty binding without changing the previous material. GetMesh,
GetMaterial and GetMaterialCount support copying bindings between components.
Gameplay cannot access mutable backend resources or skeletal joint arrays.
SetVisible only controls drawing/shadows; hidden skeletal meshes still update.
SetEnabled controls gameplay updates and render eligibility.

Select cameras through World::SetActiveCamera. Invalid SetPerspective input or
an overflowing derived projection returns false and preserves the old projection.
Camera scale is stripped during extraction, including a stable fallback for
collapsed inherited axes. Destroyed, inactive, disabled or unstarted cameras
produce an empty frame; the host stays responsive and never retains a stale scene
image indefinitely. Gameplay never synchronizes a camera or builds snapshots.

Use only GetTransform with explicitly named local/world operations. ActorRef and
ComponentRef are the retained reference types. World construction, slot tables,
phase dispatch, renderer extraction, camera backend access and raw resource access
are internal. Sources also run under the engine mutation guard: failing preparation
cannot change captured gameplay objects through engine APIs. Ordinary C++ code
must still honor ownership and avoid arbitrary external/custom-field side effects.
