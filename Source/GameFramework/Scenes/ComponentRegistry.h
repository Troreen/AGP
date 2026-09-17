#pragma once
#include "GameFramework/World/Actor.h"
#include <functional>
#include <unordered_map>
#include <stdexcept>

// Explicit registration keeps game types in the game project. Names are stable
// runtime names; source exporter IDs belong to a separate import adapter.
class ComponentRegistry
{
public:
    template<class T> void Register(std::string type)
    {
        if (myFrozen) throw std::logic_error("Component registry is frozen");
        if (type.empty() || myFactories.contains(type)) throw std::invalid_argument("Duplicate or empty component type: " + type);
        myFactories.emplace(std::move(type), [](Actor& actor, const std::string& name) { return actor.AddComponent<T>(name); });
    }
    void Freeze() { myFrozen = true; }
    bool IsFrozen() const { return myFrozen; }
    bool Contains(const std::string& type) const { return myFactories.contains(type); }
    Component* Create(const std::string& type, Actor& actor, const std::string& name) const
    {
        if (!myFrozen) throw std::logic_error("Freeze registrations before scene construction");
        auto it = myFactories.find(type);
        if (it == myFactories.end()) throw std::invalid_argument("Unknown component type: " + type);
        return it->second(actor, name);
    }
private:
    bool myFrozen = false;
    std::unordered_map<std::string, std::function<Component*(Actor&, const std::string&)>> myFactories;
};
