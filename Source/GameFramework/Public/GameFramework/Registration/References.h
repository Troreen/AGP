#pragma once
#include "GameFramework/World.h"

// Connection is a validation pass, not initialization. All configured peers exist,
// including forward references. Save handles returned here for later callbacks.
class References
{
public:
	References(Component& source, SceneDiagnostics& diagnostics) : mySource(source), myDiagnostics(diagnostics)
	{
	}

	void Error(std::string property, std::string message)
	{
		auto diagnostic = mySource.mySourceDiagnostic;
		if (diagnostic.Actor.empty())
		{
			diagnostic.Actor = mySource.GetOwner()->GetName();
		}
		if (diagnostic.Component.empty())
		{
			diagnostic.Component = mySource.GetName();
		}
		if (!property.empty())
		{
			diagnostic.Property = diagnostic.Property.empty() ? std::move(property) : diagnostic.Property + "." + property;
		}
		diagnostic.Message = std::move(message);
		diagnostic.Code = "dependency";
		diagnostic.Phase = "resolve";
		myDiagnostics.push_back(std::move(diagnostic));
	}

	template <class T> ComponentRef<T> Require(std::string name = {})
	{
		return Resolve<T>(*mySource.GetOwner(), name, false);
	}

	// No name means an optional type-only lookup. An explicitly supplied name must resolve.
	template <class T> ComponentRef<T> Optional(std::string name = {})
	{
		return Resolve<T>(*mySource.GetOwner(), name, name.empty());
	}

private:
	template <class T> ComponentRef<T> Resolve(Actor& actor, const std::string& name, bool allowAbsent)
	{
		T* found = nullptr;
		size_t count = 0;
		for (auto* c : actor.GetComponents<Component>())
		{
			if (c->IsPendingDestroy() || (!name.empty() && c->GetName() != name))
			{
				continue;
			}
			if (auto* typed = dynamic_cast<T*>(c))
			{
				found = typed;
				++count;
			}
		}
		if (count == 1)
		{
			return found->template GetRef<T>();
		}
		if (count || !allowAbsent)
		{
			Error(name, count ? "Ambiguous component dependency" : "Missing component or incorrect type");
		}
		return {};
	}

	Component& mySource;
	SceneDiagnostics& myDiagnostics;
};
