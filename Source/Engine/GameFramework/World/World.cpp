#include "GameFramework/World/World.h"
#include "GameFramework/Components/CameraComponent.h"
#include "Maths.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace
{
	constexpr float MaxFrameDeltaSeconds = 0.25f;
}

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
	std::unique_ptr<Actor> actor(new Actor(*this, std::move(name)));
	Actor* spawnedActor = actor.get();
	myActors.push_back(std::move(actor));
	return spawnedActor;
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
		for (Component* component : CollectFrame())
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
	deltaTime = CU::IsFinite(deltaTime) ? CU::Clamp(deltaTime, 0.0f, MaxFrameDeltaSeconds) : 0.0f;
	myUpdating = true;
	try
	{
		RemoveDestroyed();
		const std::vector<Component*> frame = CollectFrame();
		for (Component* component : frame)
		{
			if (!component->myBegun && !component->IsPendingDestroy() && !component->GetOwner()->IsPendingDestroy())
			{
				component->myBegun = true;
				component->BeginPlay();
			}
		}
		for (Component* component : frame)
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
	std::vector<std::unique_ptr<Actor>> actors = std::move(myActors);
	actors.clear();
	myClearing = false;
}
