#pragma once
#include <cstdint>
#include <memory>

class Actor;
class Component;
class World;
class ActorHandle;
template<class T> class ComponentHandle;
namespace GameFrameworkInternal { struct ObjectSlots; }

// Type-erased identity is implementation storage, never a conversion API.
class ObjectHandle
{
private:
    Actor* ResolveActor() const;
    Component* ResolveComponent() const;
    std::weak_ptr<GameFrameworkInternal::ObjectSlots> mySlots;
    size_t myIndex = 0;
    uint64_t myGeneration = 0;
    friend class World;
    friend class ActorHandle;
    template<class T> friend class ComponentHandle;
};

class ActorHandle : private ObjectHandle
{
public:
    ActorHandle() = default;
    Actor* Get() const { return ResolveActor(); }
    explicit operator bool() const { return Get() != nullptr; }
private:
    explicit ActorHandle(const ObjectHandle& handle) : ObjectHandle(handle) {}
    friend class Actor;
};

template<class T> class ComponentHandle : private ObjectHandle
{
public:
    ComponentHandle() = default;
    T* Get() const { return dynamic_cast<T*>(ResolveComponent()); }
    explicit operator bool() const { return Get() != nullptr; }
private:
    explicit ComponentHandle(const ObjectHandle& handle) : ObjectHandle(handle) {}
    friend class Component;
};

using ActorRef = ActorHandle;
template<class T> using ComponentRef = ComponentHandle<T>;
