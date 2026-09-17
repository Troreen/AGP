#include "GameFramework/World/World.h"
#include "GameFramework/Components/CameraComponent.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

World::~World()
{
	Clear();
}

Actor* World::SpawnActor(std::string name)
{
	if (myClearing)
	{
		throw std::logic_error("Cannot spawn during World cleanup");
	}
	if (name.empty() || FindActor(name))
	{
		throw std::invalid_argument("Actor name must be nonempty and unique: " + name);
	}
	auto actor = std::unique_ptr<Actor>(new Actor(*this, std::move(name)));
	auto* result = actor.get();
	myActors.push_back(std::move(actor));
	return result;
}

Actor* World::FindActor(const std::string& name) const
{
	for (const auto& actor : myActors)
	{
		if (!actor->IsPendingDestroy() && actor->GetName() == name)
		{
			return actor.get();
		}
	}
	return nullptr;
}

bool World::SetActiveCamera(CameraComponent* camera)
{
	if (camera && (&camera->GetWorld() != this || camera->IsPendingDestroy() || camera->GetOwner()->IsPendingDestroy()))
	{
		return false;
	}
	myCamera = camera;
	return true;
}

CameraComponent* World::GetActiveCamera() const
{
	return myCamera && !myCamera->IsPendingDestroy() && !myCamera->GetOwner()->IsPendingDestroy() ? myCamera : nullptr;
}

void World::RemoveDestroyed()
{
	if (!GetActiveCamera())
	{
		myCamera = nullptr;
	}
	// Detach objects before callbacks so callbacks cannot invalidate vector iteration.
	std::vector<std::unique_ptr<Actor>> actors;
	std::vector<std::unique_ptr<Component>> components;
	for (auto& actor : myActors)
	{
		if (actor->IsPendingDestroy())
		{
			actors.push_back(std::move(actor));
		}
		else
		{
			for (auto& component : actor->myComponents)
			{
				if (component->IsPendingDestroy())
				{
					components.push_back(std::move(component));
				}
			}
			std::erase(actor->myComponents, nullptr);
		}
	}
	std::erase(myActors, nullptr);
	for (auto& component : components)
	{
		if (component->myBegun)
		{
			component->EndPlay();
		}
	}
	// Destroy detached components before Actors; their GetOwner still refers to live memory.
	components.clear();
	actors.clear();
}

std::vector<Component*> World::CollectFrame()
{
	std::vector<Component*> frame;
	for (const auto& actor : myActors)
	{
		if (!actor->IsPendingDestroy())
		{
			for (const auto& component : actor->myComponents)
			{
				if (!component->IsPendingDestroy())
				{
					frame.push_back(component.get());
				}
			}
		}
	}
	return frame;
}

void World::BeginPlay()
{
	if (myUpdating || myClearing)
	{
		throw std::logic_error("World lifecycle cannot run recursively");
	}
	myUpdating = true;
	try
	{
		// Freeze this list: additions made by BeginPlay wait for the next update.
		for (auto* component : CollectFrame())
		{
			if (!component->myBegun && !component->IsPendingDestroy() && !component->GetOwner()->IsPendingDestroy())
			{
				component->myBegun = true;
				component->BeginPlay();
			}
		}
	}
	catch (...)
	{
		myUpdating = false;
		throw;
	}
	myUpdating = false;
}

void World::Update(float deltaTime)
{
	if (myUpdating || myClearing)
	{
		throw std::logic_error("World lifecycle cannot run recursively");
	}
	deltaTime = std::isfinite(deltaTime) ? std::clamp(deltaTime, 0.f, .25f) : 0.f;
	myUpdating = true;
	try
	{
		RemoveDestroyed();
		const auto frame = CollectFrame();
		for (auto* component : frame)
		{
			if (!component->myBegun && !component->IsPendingDestroy() && !component->GetOwner()->IsPendingDestroy())
			{
				component->myBegun = true;
				component->BeginPlay();
			}
		}
		for (auto* component : frame)
		{
			if (component->IsEnabled() && component->GetOwner()->IsActive())
			{
				component->Update(deltaTime);
			}
		}
	}
	catch (...)
	{
		myUpdating = false;
		throw;
	}
	myUpdating = false;
}

void World::Clear()
{
	if (myUpdating)
	{
		throw std::logic_error("Cannot clear World from a component callback");
	}
	if (myClearing)
	{
		return;
	}
	myClearing = true;
	myCamera = nullptr;
	auto actors = std::move(myActors);
	actors.clear();
	myClearing = false;
}
