#include "GameFramework/World/Actor.h"
#include "GameFramework/World/World.h"
#include <stdexcept>

Actor::Actor(World& world, std::string name) : myName(std::move(name)), myWorld(&world)
{
}

Actor::~Actor()
{
	// Reverse order keeps earlier dependencies alive through later EndPlay calls.
	for (auto it = myComponents.rbegin(); it != myComponents.rend(); ++it)
	{
		if ((*it)->myBegun)
		{
			(*it)->EndPlay();
		}
	}
}

void Actor::Attach(std::unique_ptr<Component> component, std::string name)
{
	if (myDestroyed || myWorld->myClearing)
	{
		throw std::logic_error("Cannot add a component during destruction");
	}
	if (name.empty() || FindComponent(name))
	{
		throw std::invalid_argument("Component name must be nonempty and unique: " + name);
	}
	component->myOwner = this;
	component->myName = std::move(name);
	myComponents.push_back(std::move(component));
}

Component* Actor::FindComponent(const std::string& name) const
{
	for (const std::unique_ptr<Component>& component : myComponents)
	{
		if (!component->IsPendingDestroy() && component->GetName() == name)
		{
			return component.get();
		}
	}
	return nullptr;
}
