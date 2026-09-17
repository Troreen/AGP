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
            player->SetPosition({0, 0, dt * 100});
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

M2 supplies the safe transform facade and consistent pending lookup. M4 closes
mesh/camera/light renderer dependencies and completes physical header separation.
No physics, animation graph, networking, editor, new asset manager or input system
is part of this work. Actual Perforce scene integration requires verified team code
and fixtures; the existing FBX asset importer is not that scene importer.

See [implementation evidence](SimplifiedGameFrameworkImplementation.md) for build,
test and review results and [the plan](SimplifiedGameFrameworkPlan.md) for contracts.
