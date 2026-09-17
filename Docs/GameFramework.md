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
            player->GetTransform().SetLocalPosition({0, 0, dt * 100});
        if (game.GetInput().IsKeyPressed(Keys::ESCAPE)) game.RequestQuit();
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

Scene construction, checked references, hierarchy and replacement already exist.
ModelViewer's M1/M2 integration uses the explicitly temporary `LegacySceneBridge`
and old scene descriptions. Normal game code cannot submit candidate worlds through
GameContext. M3 replaces that integration bridge with owned SceneData and a scene-ID
service. Invalid content still follows the old Debug assertion policy until M3.

M2 implements the safe transform facade and consistent pending lookup. M4 closes
mesh/camera/light renderer dependencies and completes physical header separation.
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
