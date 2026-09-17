#pragma once
#include "GameFramework/Actor.h"
#include "GameFramework/Registration/SceneReader.h"
#include <functional>
#include <unordered_map>
#include <stdexcept>

namespace GameFrameworkInternal { struct RegistryAccess; }
class SceneBuilder;

// Stable authored type names map to one factory and one checked property reader.
// Direct C++ AddComponent does not need registration.
class ComponentRegistry
{
public:
    template<class T> void Register(std::string type, std::function<void(T&, SceneReader&)> reader = {})
    {
        RegisterFactory<T>(std::move(type), [] { return std::make_unique<T>(); }, std::move(reader));
    }
    // Factories create unattached components. Owner/services are unavailable, and
    // framework mutation through captured objects is rejected during construction.
    template<class T> void RegisterFactory(std::string type,
        std::function<std::unique_ptr<T>()> factory,
        std::function<void(T&, SceneReader&)> reader = {})
    {
        static_assert(std::is_base_of_v<Component, T>);
        if (myFrozen) throw std::logic_error("Component registry is frozen");
        if (type.empty() || myTypes.contains(type)) throw std::invalid_argument("Duplicate or empty component type: " + type);
        if (!factory) throw std::invalid_argument("A component factory is required");
        Entry entry;
        entry.Factory = [factory = std::move(factory)]() -> std::unique_ptr<Component> { return factory(); };
        entry.Reader = [reader = std::move(reader)](Component& component, SceneReader& fields)
        { if (reader) reader(dynamic_cast<T&>(component), fields); };
        myTypes.emplace(std::move(type), std::move(entry));
    }
private:
    struct Entry
    {
        std::function<std::unique_ptr<Component>()> Factory;
        std::function<void(Component&, SceneReader&)> Reader;
    };
    friend struct GameFrameworkInternal::RegistryAccess;
    friend class SceneBuilder;
    void Freeze() { myFrozen = true; }
    bool IsFrozen() const { return myFrozen; }
    bool Contains(const std::string& type) const { return myTypes.contains(type); }
    Component* Create(const std::string& type, Actor& actor, const std::string& name) const;
    void Configure(const std::string& type, Component& component, SceneReader& fields) const
    { myTypes.at(type).Reader(component, fields); }
    bool myFrozen = false;
    std::unordered_map<std::string, Entry> myTypes;
};
