#pragma once
#include <cstdint>
#include <memory>
#include <vector>

class Actor;
class Component;
class World;

namespace GameFrameworkInternal
{
    // Only the gameplay owner accesses these slots. Weak ownership prevents handles
    // keeping a scene alive; generations prevent reuse from reviving old references.
    struct ObjectSlots
    {
        struct Slot { Actor* actor = nullptr; Component* component = nullptr; uint64_t generation = 1; };
        std::vector<Slot> slots;
    };
}

class ObjectHandle
{
public:
    Actor* ResolveActor() const;
    Component* ResolveComponent() const;
private:
    std::weak_ptr<GameFrameworkInternal::ObjectSlots> mySlots;
    size_t myIndex = 0;
    uint64_t myGeneration = 0;
    friend class World;
};

class ActorHandle : public ObjectHandle
{
public:
    ActorHandle() = default;
    explicit ActorHandle(const ObjectHandle& handle) : ObjectHandle(handle) {}
    Actor* Get() const { return ResolveActor(); }
    explicit operator bool() const { return Get() != nullptr; }
};

template<class T> class ComponentHandle : public ObjectHandle
{
public:
    ComponentHandle() = default;
    explicit ComponentHandle(const ObjectHandle& handle) : ObjectHandle(handle) {}
    T* Get() const { return dynamic_cast<T*>(ResolveComponent()); }
    explicit operator bool() const { return Get() != nullptr; }
};

using ActorRef = ActorHandle;
template<class T> using ComponentRef = ComponentHandle<T>;
