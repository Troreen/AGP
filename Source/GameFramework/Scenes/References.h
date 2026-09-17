#pragma once
#include "../World/World.h"

// Connection is a validation pass, not initialization. All configured peers exist,
// including forward references. Save handles returned here for later callbacks.
class References
{
public:
    References(Component& source, SceneDiagnostics& diagnostics) : mySource(source), myDiagnostics(diagnostics) {}
    void Error(std::string property, std::string message)
    { myDiagnostics.push_back({mySource.GetOwner()->GetName(), mySource.GetName(), std::move(property), std::move(message)}); }
    template<class T> ComponentHandle<T> Require(std::string name = {})
    { return Resolve<T>(*mySource.GetOwner(), name, false); }
    template<class T> ComponentHandle<T> Require(const std::string& actor, const std::string& component)
    {
        auto* owner = mySource.GetWorld().FindActor(actor);
        if (!owner) { Error(component, "Required actor not found: " + actor); return {}; }
        return Resolve<T>(*owner, component, false);
    }
    // No name means an optional type-only lookup. An explicitly supplied name must resolve.
    template<class T> ComponentHandle<T> Optional(std::string name = {})
    { return Resolve<T>(*mySource.GetOwner(), name, name.empty()); }
    template<class T> ComponentHandle<T> Optional(const std::string& actor, const std::string& component)
    {
        if (actor.empty() && component.empty()) return {};
        return Require<T>(actor,component);
    }
private:
    template<class T> ComponentHandle<T> Resolve(Actor& actor, const std::string& name, bool allowAbsent)
    {
        T* found = nullptr;
        size_t count = 0;
        for (auto* c : actor.GetComponents<Component>())
        {
            if (c->IsPendingDestroy() || (!name.empty() && c->GetName() != name)) continue;
            if (auto* typed = dynamic_cast<T*>(c)) { found = typed; ++count; }
        }
        if (count == 1) return found->template GetHandle<T>();
        if (count || !allowAbsent) Error(name, count ? "Ambiguous component dependency" : "Missing component or incorrect type");
        return {};
    }
    Component& mySource;
    SceneDiagnostics& myDiagnostics;
};

