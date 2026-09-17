#pragma once
#include <cstdint>
#include <memory>
class Actor;
class Component;
class World;
class ActorRef;
template<class T> class ComponentRef;
namespace GameFrameworkInternal
{
    struct ObjectSlots;
    class ObjectIdentity
    {
    private:
        Actor* ResolveActor() const;
        Component* ResolveComponent() const;
        std::weak_ptr<ObjectSlots> mySlots;
        size_t myIndex = 0;
        uint64_t myGeneration = 0;
        friend class ::World;
        friend class ::ActorRef;
        template<class T> friend class ::ComponentRef;
    };
}
// Non-owning checked identity. Borrow the returned pointer only in this callback.
class ActorRef : private GameFrameworkInternal::ObjectIdentity
{
public:
    ActorRef() = default;
    Actor* Get() const { return ResolveActor(); }
    explicit operator bool() const { return Get() != nullptr; }
private:
    explicit ActorRef(const GameFrameworkInternal::ObjectIdentity& identity) : ObjectIdentity(identity) {}
    friend class Actor;
};
template<class T> class ComponentRef : private GameFrameworkInternal::ObjectIdentity
{
public:
    ComponentRef() = default;
    T* Get() const { return dynamic_cast<T*>(ResolveComponent()); }
    explicit operator bool() const { return Get() != nullptr; }
private:
    explicit ComponentRef(const GameFrameworkInternal::ObjectIdentity& identity) : ObjectIdentity(identity) {}
    friend class Component;
};
