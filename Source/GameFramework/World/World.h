#pragma once
#include "Actor.h"
#include "GameFramework/Input/GameInput.h"
#include "GameFramework/Scenes/SceneDiagnostic.h"
#include <functional>

// One serialized gameplay owner controls a world. Requests made inside hooks are
// queued; the host flushes once before the next frame's fixed steps.
class World
{
public:
    enum class State { Constructing, Prepared, Active, Ending };
    explicit World(const GameInput* input = nullptr);
    ~World();
    World(const World&) = delete;
    World& operator=(const World&) = delete;
    Actor* CreateActor(std::string name);
    Actor* FindActor(const std::string& name) const;
    void DestroyActor(Actor& actor);
    void DestroyComponent(Component& component);
    bool Prepare(SceneDiagnostics& diagnostics);
    void Activate();
    bool Flush(SceneDiagnostics& diagnostics);
    void Shutdown() noexcept;
    void FixedUpdate(float delta);
    void Update(float delta);
    State GetState() const { return myState; }
    bool AcceptsChanges() const { return myState != State::Ending; }
    const GameInput& GetInput() const { return myInput ? *myInput : myEmptyInput; }
    const std::vector<std::unique_ptr<Actor>>& GetActors() const { return myActors; }
    // Engine hierarchy commands capture handles, never borrowed object pointers.
    void QueueStructure(std::function<void(SceneDiagnostics&)> command) { myCommands.push_back(std::move(command)); }
private:
    ObjectHandle Allocate(Actor* actor, Component* component);
    void Invalidate(const ObjectHandle& handle);
    void Attach(Actor& actor, std::unique_ptr<Component> component);
    void CollectDestroyed() noexcept;
    bool ConnectBatch(const std::vector<Component*>& batch, SceneDiagnostics& diagnostics);
    void BeginBatch(const std::vector<Component*>& batch);
    State myState = State::Constructing;
    bool myInBoundary = false;
    const GameInput* myInput;
    GameInput myEmptyInput;
    std::shared_ptr<GameFrameworkInternal::ObjectSlots> mySlots;
    std::vector<std::unique_ptr<Actor>> myActors;
    std::vector<std::unique_ptr<Actor>> myPendingActors;
    std::vector<Component*> myActivationOrder;
    std::vector<std::function<void(SceneDiagnostics&)>> myCommands;
    friend class Actor;
};
