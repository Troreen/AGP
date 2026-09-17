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
#include <GameFramework/Registration/References.h>
#include <GameFramework/Registration/SceneReader.h>
#include <GameFramework/SceneService.h>
#include <GameFramework/GameTime.h>
#include <GameFramework/Transform.h>
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
static_assert(std::is_final_v<Actor> && !std::is_constructible_v<Actor,std::string>);
static_assert(!std::is_copy_constructible_v<Transform> && !std::is_move_constructible_v<Transform>);
static_assert(!std::is_copy_assignable_v<Transform> && !std::is_move_assignable_v<Transform>);
template<class T> concept CanSetRawParent = requires(T& value) { value.SetParent(&value); };
static_assert(!CanSetRawParent<Transform>);
static_assert(!std::is_constructible_v<ActorRef,ComponentRef<Component>>);
static_assert(!std::is_constructible_v<ComponentRef<SceneComponent>,ActorRef>);
static_assert(!std::is_constructible_v<ComponentRef<SceneComponent>,ComponentRef<Component>>);

class GameplayProbe final : public Component
{
    void Update(float) override
    {
        if (GetInput().IsKeyPressed(Keys::R)) GetOwner()->SetActive(false);
    }
};

class FollowTarget final : public Component
{
public:
    ActorRef Target;
private:
    void LateUpdate(float) override
    {
        if (auto* target=Target.Get())
            GetOwner()->GetTransform().SetWorldPosition(target->GetTransform().GetWorldPosition());
    }
};

class NeedsOffset final : public Component
{
    void ResolveReferences(References& references) override {myOffset=references.Require<SceneComponent>("Offset");}
    ComponentRef<SceneComponent> myOffset;
};

class AuthoredFollow final : public Component
{
public:
    ActorRef Target;
    float Speed=25;
};

class MinimalGame final : public IGame
{
public:
    void RegisterComponents(ComponentRegistry& types) override
    {
        types.Register<AuthoredFollow>("sample.Follow",[](AuthoredFollow& component,SceneReader& data)
        {
            component.Speed=data.OptionalFloat("speed",25);
            data.BindActor("target",component.Target,ReferenceRequirement::Required);
        });
    }
    void Initialize(GameContext& game) override
    {
        auto* actor=game.GetWorld().SpawnActor("Example");
        auto* behavior=actor->AddComponent<GameplayProbe>("Behavior");
        actor->GetTransform().SetLocalPosition({100,0,250});
        auto* unnamed=actor->AddComponent<SceneComponent>();
        unnamed->GetTransform().SetLocalScale({2,2,2});
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

void CompileSceneRequests(GameContext& game)
{
    game.GetScenes().Load(SceneId{"Levels/Town"});
    game.GetScenes().Reload();
}
