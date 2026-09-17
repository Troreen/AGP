#pragma once
#include "GameFramework/Actor.h"
#include "GameFramework/GameInput.h"
#include "GameFramework/SceneDiagnostic.h"
#include <functional>

class CameraComponent;
class SceneBuilder;
class SceneComponent;
class SceneService;
class AssetLookup;
class GameTime;
namespace GameFrameworkInternal { class WorldAccess; }

// The session owns a world and advances its objects automatically. Pointers
// returned by spawn/find are temporary borrows; retain object refs across callbacks.
class World
{
public:
    ~World();
    World(const World&) = delete;
    World& operator=(const World&) = delete;
    Actor* SpawnActor(std::string name);
    Actor* FindActor(const std::string& name) const;
    std::vector<Actor*> FindActors(const std::string& name) const;
    void DestroyActor(Actor& actor);
    void DestroyComponent(Component& component);
    const GameInput& GetInput() const { return myInput ? *myInput : myEmptyInput; }
    SceneService& GetScenes() const;
    const AssetLookup& GetAssets() const;
    const GameTime& GetTime() const;
    bool SetActiveCamera(CameraComponent* camera);
    CameraComponent* GetActiveCamera() const;
private:
    explicit World(const GameInput* input = nullptr);
    enum class State { Constructing, Prepared, Active, Ending };
    bool Prepare(SceneDiagnostics& diagnostics);
    void Activate();
    bool Flush(SceneDiagnostics& diagnostics);
    void Shutdown() noexcept;
    void FixedUpdate(float delta);
    void Update(float delta);
    State GetState() const { return myState; }
    bool AcceptsChanges() const { return !myClosing && myState != State::Ending; }
    const std::vector<std::unique_ptr<Actor>>& GetActors() const { return myActors; }
    void EnsureMutationAllowed() const;
    void EnsureComponentMutationAllowed(const Component*) const;
    GameFrameworkInternal::ObjectIdentity Allocate(Actor* actor, Component* component);
    void Invalidate(const GameFrameworkInternal::ObjectIdentity& handle);
    void Attach(Actor& actor, std::unique_ptr<Component> component);
    void CollectDestroyed() noexcept;
    bool ConnectBatch(const std::vector<Component*>& batch, SceneDiagnostics& diagnostics);
    void BeginBatch(const std::vector<Component*>& batch);
    State myState = State::Constructing;
    // Closing suppresses new work while Shutdown callbacks still borrow the world.
    // Ending is reserved for the later lifecycle cleanup performed by Shutdown.
    bool myClosing = false;
    bool myInBoundary = false;
    const GameInput* myInput;
    GameInput myEmptyInput;
    SceneService* myScenes = nullptr;
    const AssetLookup* myAssets = nullptr;
    const GameTime* myTime = nullptr;
    std::shared_ptr<GameFrameworkInternal::ObjectSlots> mySlots;
    std::vector<std::unique_ptr<Actor>> myActors;
    std::vector<std::unique_ptr<Actor>> myPendingActors;
    std::vector<Component*> myActivationOrder;

    ComponentRef<CameraComponent> myCamera;
    friend class Actor;
    friend class Component;
    friend class Transform;
    friend class SceneComponent;
    friend class SceneBuilder;
    friend class GameFrameworkInternal::WorldAccess;
};
