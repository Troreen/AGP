#include "World.h"

#include "GameFrameworkLog.h"
#include <cassert>
#include <utility>

World::World() = default;

World::~World() = default;

Actor* World::CreateActor(std::string aName)
{
	if (!CanAddActorName(aName))
	{
		ReportDuplicateActorName(aName);
		return nullptr;
	}

	auto actor = std::make_unique<Actor>(std::move(aName));
	Actor* rawActor = actor.get();
	rawActor->SetWorld(this);

	myActors.push_back(std::move(actor));
	return rawActor;
}

Actor* World::FindActor(const std::string& aName) const
{
	for (const std::unique_ptr<Actor>& actor : myActors)
	{
		if (actor->GetName() == aName)
		{
			return actor.get();
		}
	}

	return nullptr;
}

void World::Update(float aDeltaTime)
{
	for (std::unique_ptr<Actor>& actor : myActors)
	{
		actor->Update(aDeltaTime);
	}

	for (std::unique_ptr<Actor>& actor : myActors)
	{
		actor->LateUpdate(aDeltaTime);
	}
}

const std::vector<std::unique_ptr<Actor>>& World::GetActors() const
{
	return myActors;
}

bool World::CanAddActorName(const std::string& aName) const
{
	return !aName.empty() && FindActor(aName) == nullptr;
}

void World::ReportDuplicateActorName(const std::string& aName) const
{
	GFLOG(Error, "World could not create actor '{}'. Actor names must be non-empty and unique.", aName);
	assert(false && "Duplicate or empty actor name");
}
