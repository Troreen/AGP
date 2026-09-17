#pragma once
#include "Actor.h"
#include "../Input/GameInput.h"
#include "../Scenes/SceneDiagnostic.h"
#include <functional>

class CameraComponent;
class SceneBuilder;
class SceneComponent;
namespace GameFrameworkInternal { class WorldAccess; }

// The session owns a world and advances its objects automatically. Pointers
// returned by spawn/find are temporary borrows; retain object refs across callbacks.
class World
{
public:
    explicit World(const GameInput* input = nullptr);
    ~World();
    World(const World&) = delete;
    World& operator=(const World&) = delete;
    Actor* CreateActor(std::string name);
    Actor* SpawnActor(std::string name) { return CreateActor(std::move(name)); }
    Actor* FindActor(const std::string& name) const;
    void DestroyActor(Actor& actor);
    void DestroyComponent(Component& component);
    const GameInput& GetInput() const { return myInput ? *myInput : myEmptyInput; }
    bool SetActiveCamera(CameraComponent* camera);
    CameraComponent* GetActiveCamera() const;
private:
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
    // Engine hierarchy commands capture handles, never borrowed object pointers.
    void QueueStructure(std::function<void(SceneDiagnostics&)> command) { myCommands.push_back(std::move(command)); }
    ObjectHandle Allocate(Actor* actor, Component* component);
    void Invalidate(const ObjectHandle& handle);
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
    std::shared_ptr<GameFrameworkInternal::ObjectSlots> mySlots;
    std::vector<std::unique_ptr<Actor>> myActors;
    std::vector<std::unique_ptr<Actor>> myPendingActors;
    std::vector<Component*> myActivationOrder;
    std::vector<std::function<void(SceneDiagnostics&)>> myCommands;
    ComponentHandle<CameraComponent> myCamera;
    friend class Actor;
    friend class SceneComponent;
    friend class SceneBuilder;
    friend class GameFrameworkInternal::WorldAccess;
};
