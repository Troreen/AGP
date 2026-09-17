#include "GameFramework/Components/CameraComponent.h"
#include "World.h"
#include "GameFramework/Scenes/References.h"
#include "GameFramework/Diagnostics/GameFrameworkLog.h"
#include <algorithm>
#include "GameFramework/Components/SceneComponent.h"
#include <stdexcept>
#include <limits>
#include "GameFramework/Runtime/Internal/ObjectSlots.h"
#include "GameFramework/Runtime/Internal/WorldAccess.h"
#include "GameFramework/Assets/AssetRefs.h"
#include "GameFramework/Runtime/GameTime.h"
namespace
{
    thread_local bool resolvingReferences = false;
    thread_local const Component* configuringComponent = nullptr;
    thread_local uint64_t rejectedMutations = 0;
}
void World::EnsureMutationAllowed() const
{
    if (resolvingReferences || configuringComponent)
    { ++rejectedMutations; throw std::logic_error("Structural or session mutation is forbidden during scene configuration/resolution"); }
}
void World::EnsureComponentMutationAllowed(const Component* component) const
{
    if (resolvingReferences || (configuringComponent && configuringComponent != component))
    { ++rejectedMutations; throw std::logic_error("Only the reader target can be configured; resolution is read-only"); }
}
void GameFrameworkInternal::WorldAccess::Configure(Component& component, const std::function<void()>& callback)
{
    struct Guard
    {
        const Component* Previous = configuringComponent;
        explicit Guard(const Component& target) { configuringComponent = &target; }
        ~Guard() { configuringComponent = Previous; }
    } guard(component);
    const auto before = rejectedMutations;
    callback();
    if (rejectedMutations != before) throw std::logic_error("Forbidden mutation attempted by scene reader");
}
void GameFrameworkInternal::WorldAccess::Construct(const std::function<void()>& callback)
{
    struct Guard
    {
        bool Previous = resolvingReferences;
        Guard() { resolvingReferences = true; }
        ~Guard() { resolvingReferences = Previous; }
    } guard;
    const auto before = rejectedMutations;
    callback();
    if (rejectedMutations != before) throw std::logic_error("Forbidden mutation attempted by component factory");
}
SceneService& World::GetScenes() const
{ if (!myScenes) throw std::logic_error("World has no scene service"); return *myScenes; }
const AssetLookup& World::GetAssets() const { static const AssetLookup empty; return myAssets ? *myAssets : empty; }
const GameTime& World::GetTime() const { static const GameTime empty; return myTime ? *myTime : empty; }

Actor* ObjectHandle::ResolveActor() const
{
    auto slots = mySlots.lock();
    if (!slots || myIndex >= slots->slots.size()) return nullptr;
    const auto& slot = slots->slots[myIndex];
    return slot.generation == myGeneration && slot.actor && !slot.actor->IsPendingDestroy() ? slot.actor : nullptr;
}
Component* ObjectHandle::ResolveComponent() const
{
    auto slots = mySlots.lock();
    if (!slots || myIndex >= slots->slots.size()) return nullptr;
    const auto& slot = slots->slots[myIndex];
    return slot.generation == myGeneration && slot.component && !slot.component->IsPendingDestroy() ? slot.component : nullptr;
}
World::World(const GameInput* input) : myInput(input), mySlots(std::make_shared<GameFrameworkInternal::ObjectSlots>()) {}
World::~World() { Shutdown(); }
ObjectHandle World::Allocate(Actor* actor, Component* component)
{
    size_t index = 0;
    while (index < mySlots->slots.size() && (mySlots->slots[index].actor || mySlots->slots[index].component || mySlots->slots[index].generation == 0)) ++index;
    if (index == mySlots->slots.size()) mySlots->slots.emplace_back();
    auto& slot = mySlots->slots[index]; slot.actor = actor; slot.component = component;
    ObjectHandle handle; handle.mySlots = mySlots; handle.myIndex = index; handle.myGeneration = slot.generation;
    return handle;
}
void World::Invalidate(const ObjectHandle& handle)
{
    auto& slot = mySlots->slots[handle.myIndex]; slot.actor = nullptr; slot.component = nullptr; slot.generation = slot.generation == std::numeric_limits<uint64_t>::max() ? 0 : slot.generation + 1;
}
Actor* World::CreateActor(std::string name)
{
    EnsureMutationAllowed();
    if (!AcceptsChanges()) return nullptr;
    auto actor = std::unique_ptr<Actor>(new Actor(std::move(name)));
    auto* raw = actor.get(); raw->SetWorld(this); raw->myHandle = Allocate(raw, nullptr);
    (myState == State::Constructing && !myInBoundary ? myActors : myPendingActors).push_back(std::move(actor));
    return raw;
}
std::vector<Actor*> World::FindActors(const std::string& name) const
{
    std::vector<Actor*> result;
    for (const auto* list : {&myActors, &myPendingActors})
        for (const auto& actor : *list)
            if (!actor->myPendingDestroy && actor->GetName() == name) result.push_back(actor.get());
    return result;
}
Actor* World::FindActor(const std::string& name) const
{
    auto matches = FindActors(name);
    if (matches.size() > 1) GFLOG(Warning, "Ambiguous actor display name: {}", name);
    return matches.size() == 1 ? matches.front() : nullptr;
}
void World::Attach(Actor& actor, std::unique_ptr<Component> component)
{
    if (auto* spatial = dynamic_cast<SceneComponent*>(component.get())) { spatial->myTransform.myWorld = this; spatial->myTransform.myComponent = component.get(); spatial->myTransform.myValue.SetParent(&actor.myTransform.myValue); }
    component->myHandle = Allocate(nullptr, component.get());
    (myState == State::Constructing && !myInBoundary ? actor.myComponents : actor.myPendingComponents).push_back(std::move(component));
}
void World::DestroyActor(Actor& actor)
{
    EnsureMutationAllowed();
    if (actor.GetWorld() != this || actor.myPendingDestroy) return;
    // Mark descendants before invalidating the parent handle used to find them.
    for (auto* list : {&myActors, &myPendingActors})
        for (auto& child : *list) if (child->GetParent() == &actor) DestroyActor(*child);
    actor.myPendingDestroy = true;
    actor.RemoveAllComponents();
}
void World::DestroyComponent(Component& component)
{
    EnsureMutationAllowed();
    if (&component.GetWorld() != this || component.myPendingDestroy) return;
    if (auto* spatial = dynamic_cast<SceneComponent*>(&component))
        for (auto* list : {&component.GetOwner()->myComponents, &component.GetOwner()->myPendingComponents})
            for (auto& child : *list)
                if (auto* s = dynamic_cast<SceneComponent*>(child.get()); s && s->GetParent() == spatial) DestroyComponent(*s);
    component.myPendingDestroy = true;
}
bool World::ConnectBatch(const std::vector<Component*>& batch, SceneDiagnostics& diagnostics)
{
    const auto before = diagnostics.size();
    struct Guard { bool previous = resolvingReferences; Guard() { resolvingReferences = true; } ~Guard() { resolvingReferences = previous; } } guard;
    for (auto* c : batch)
    {
        if (c->myPendingDestroy) continue;
        References context(*c, diagnostics);
        const auto rejectedBefore = rejectedMutations;
        const auto errorsBefore = diagnostics.size();
        try { c->ResolveReferences(context); c->myConnected = true; }
        catch (const std::bad_alloc&) { throw; }
        catch (const std::exception& e) { context.Error({}, e.what()); }
        catch (...) { context.Error({}, "Unknown exception in ResolveReferences"); }
        if (rejectedMutations != rejectedBefore && diagnostics.size() == errorsBefore)
            context.Error({}, "Mutation attempted during ResolveReferences");
    }
    return diagnostics.size() == before;
}
void World::BeginBatch(const std::vector<Component*>& batch)
{
    myActivationOrder.reserve(myActivationOrder.size() + batch.size());
    for (auto* c : batch)
        if (!c->myPendingDestroy && c->myConnected && !c->myBegun)
        {
            c->BeginPlay(); c->myBegun = true; myActivationOrder.push_back(c);
        }
}
bool World::Prepare(SceneDiagnostics& diagnostics)
{
    if (myState != State::Constructing) throw std::logic_error("Prepare requires a constructing world");
    myInBoundary = true;
    std::vector<Component*> batch;
    for (auto& a : myActors) for (auto& c : a->myComponents) batch.push_back(c.get());
    const bool valid = ConnectBatch(batch, diagnostics);
    myInBoundary = false;
    if (valid) myState = State::Prepared;
    return valid;
}
void World::Activate()
{
    if (myState != State::Prepared) throw std::logic_error("Activate requires a prepared world");
    myState = State::Active;
    for (auto& a : myActors) { a->myAdmitted = true; for (auto& c : a->myComponents) c->myAdmitted = true; }
    std::vector<Component*> batch;
    for (auto& a : myActors) for (auto& c : a->myComponents) batch.push_back(c.get());
    BeginBatch(batch);
}
bool World::Flush(SceneDiagnostics& diagnostics)
{
    if (myState != State::Active) return false;
    const auto errorCount = diagnostics.size();
    // Freeze before any teardown or connection callback; reentrant additions wait.
    auto actors = std::move(myPendingActors); myPendingActors.clear();
    std::vector<std::pair<Actor*, std::vector<std::unique_ptr<Component>>>> additions;
    for (auto& a : myActors) { additions.emplace_back(a.get(), std::move(a->myPendingComponents)); a->myPendingComponents.clear(); }
    for (auto& a : actors) { additions.emplace_back(a.get(), std::move(a->myPendingComponents)); a->myPendingComponents.clear(); }
    std::vector<Actor*> addedActors;
    for (auto& a : actors) { addedActors.push_back(a.get()); myActors.push_back(std::move(a)); }
    std::vector<Component*> batch;
    for (auto& [a, components] : additions) for (auto& c : components)
    { batch.push_back(c.get()); a->myComponents.push_back(std::move(c)); }
    const bool valid = ConnectBatch(batch, diagnostics) && diagnostics.size() == errorCount;
    if (!valid)
    {
        for (auto* c : batch) DestroyComponent(*c);
        for (auto* a : addedActors) DestroyActor(*a);
    }
    CollectDestroyed();
    if (valid)
    {
        for (auto& a : myActors) { a->myAdmitted = true; for (auto& c : a->myComponents) c->myAdmitted = true; }
        // Destroyed objects have been collected; resolve saved identities instead.
        std::vector<Component*> live;
        for (auto& a : myActors) for (auto& c : a->myComponents)
            if (c->myConnected && !c->myBegun) live.push_back(c.get());
        BeginBatch(live);
    }
    return valid;
}
void World::CollectDestroyed() noexcept
{
    // Move the frozen removal set out first. Cleanup hooks may request later work,
    // but cannot invalidate containers currently being traversed.
    std::vector<std::unique_ptr<Component>> removedComponents;
    std::vector<std::unique_ptr<Actor>> removedActors;
    for (auto* list : {&myActors, &myPendingActors})
    {
        for (auto& actor : *list)
            for (auto* components : {&actor->myComponents, &actor->myPendingComponents})
            {
                for (auto& c : *components) if (c->myPendingDestroy) removedComponents.push_back(std::move(c));
                std::erase_if(*components, [](const auto& c) { return !c; });
            }
        for (auto& a : *list) if (a->myPendingDestroy) removedActors.push_back(std::move(a));
        std::erase_if(*list, [](const auto& a) { return !a; });
    }
    auto removed = [&](Component* c)
    { return std::any_of(removedComponents.begin(),removedComponents.end(),[&](const auto& owned) { return owned.get() == c; }); };
    std::vector<Component*> endOrder;
    for (auto it = myActivationOrder.rbegin(); it != myActivationOrder.rend(); ++it) if (removed(*it)) endOrder.push_back(*it);
    std::erase_if(myActivationOrder,removed);
    // Reverse activation is the default; deeper attachments end before ancestors.
    auto depth = [](Component* c)
    {
        size_t n = 0;
        const CommonUtilities::Transform* t = &c->GetOwner()->myTransform.myValue;
        if (auto* spatial = dynamic_cast<SceneComponent*>(c)) t = &spatial->myTransform.myValue;
        while (t->GetParent()) { ++n; t = t->GetParent(); } return n;
    };
    std::stable_sort(endOrder.begin(),endOrder.end(),[&](auto* a,auto* b) { return depth(a) > depth(b); });
    for (auto* c : endOrder)
    {
        c->myBegun = false;
        try { c->EndPlay(); } catch (...) { GFLOG(Error, "EndPlay failed during cleanup"); }
    }
    std::stable_sort(removedComponents.begin(),removedComponents.end(),[&](const auto& a,const auto& b) { return depth(a.get()) > depth(b.get()); });
    for (auto& c : removedComponents)
    {
        Invalidate(c->myHandle);
    }
    // Keep all objects allocated until every parent pointer is detached.
    for (auto& c : removedComponents) if (auto* spatial = dynamic_cast<SceneComponent*>(c.get())) spatial->myTransform.myValue.SetParent(nullptr);
    for (auto& a : removedActors) { a->myTransform.myValue.SetParent(nullptr); Invalidate(a->myHandle); }
    removedComponents.clear(); // Component destructors may still inspect their owner.
    removedActors.clear();
}
void World::Shutdown() noexcept
{
    if (myState == State::Ending) return;
    myState = State::Ending;
    for (auto* list : {&myActors, &myPendingActors}) for (auto& a : *list) DestroyActor(*a);
    CollectDestroyed(); mySlots.reset();
}
void World::FixedUpdate(float delta)
{
    if (myState != State::Active) return;
    for (auto& a : myActors) a->FixedUpdate(delta);
}
void World::Update(float delta)
{
    if (myState != State::Active) return;
    for (auto& a : myActors) a->Update(delta);
    for (auto& a : myActors) a->LateUpdate(delta);
}

bool World::SetActiveCamera(CameraComponent* camera)
{
    EnsureMutationAllowed();
    if (camera && (&camera->GetWorld() != this || camera->IsPendingDestroy())) return false;
    myCamera = camera ? camera->GetHandle<CameraComponent>() : ComponentHandle<CameraComponent>{};
    return true;
}
CameraComponent* World::GetActiveCamera() const { return myCamera.Get(); }
