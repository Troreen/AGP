#include <GameFramework/GameApplication.h>
#include <GameFramework/IGame.h>
#include <GameFramework/GameContext.h>
#include <GameFramework/World.h>
#include <GameFramework/Actor.h>
#include <GameFramework/Component.h>
#include <GameFramework/SceneComponent.h>
#include <GameFramework/GameInput.h>
#include <GameFramework/ObjectRef.h>
#include <GameFramework/Registration/ComponentRegistry.h>
#include <type_traits>

// This translation unit has only Public and shared-math include directories.
// Access checks must remain dependent expressions so inaccessible members are false.
template<class T> concept CanTick = requires(T& value) { value.Update(0.01f); };
template<class T> concept CanActivate = requires(T& value) { value.Activate(); };
template<class T> concept CanShutdown = requires(T& value) { value.Shutdown(); };
template<class T> concept CanPrepare = requires { &T::Prepare; };
template<class T> concept CanFlush = requires { &T::Flush; };
template<class T> concept CanQueue = requires { &T::QueueStructure; };
template<class T> concept CanFreeze = requires(T& value) { value.Freeze(); };
template<class T> concept CanSeeState = requires(T& value) { value.GetState(); };
template<class T> concept CanSeeOwnedActors = requires(T& value) { value.GetActors(); };
template<class T> concept CanSeeOwnedComponents = requires(T& value) { value.GetComponents(); };
template<class T> concept CanClearInput = requires(T& value) { value.ClearPressed(); };
template<class T> concept CanMergeInput = requires(T& value) { value.Merge(value); };
static_assert(!CanTick<World> && !CanTick<Actor>);
static_assert(!CanActivate<World> && !CanShutdown<World> && !CanSeeState<World>);
static_assert(!CanPrepare<World> && !CanFlush<World> && !CanQueue<World>);
static_assert(!CanFreeze<ComponentRegistry>);
static_assert(!CanSeeOwnedActors<World> && !CanSeeOwnedComponents<Actor>);
static_assert(!CanClearInput<GameInput> && !CanMergeInput<GameInput>);
static_assert(!std::is_copy_constructible_v<World>);

class GameplayProbe final : public Component
{
    void Update(float) override
    {
        if (GetInput().IsKeyPressed(Keys::R)) GetOwner()->SetActive(false);
    }
};

class MinimalGame final : public IGame
{
public:
    void Initialize(GameContext& game) override
    {
        auto* actor=game.GetWorld().SpawnActor("Example");
        auto* behavior=actor->AddComponent<GameplayProbe>("Behavior");
        myActor=actor->GetRef();
        myBehavior=behavior->GetRef<GameplayProbe>();
    }
    void Update(GameContext& game,float) override
    {
        if (auto* actor=myActor.Get()) actor->SetActive(true);
        game.RequestQuit();
    }
private:
    ActorRef myActor;
    ComponentRef<GameplayProbe> myBehavior;
};

int CompileGameplayExample()
{
    MinimalGame game;
    GameApplication::Config config;
    return GameApplication{}.Run(game,config);
}
