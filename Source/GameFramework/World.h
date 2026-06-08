#pragma once

#include "Actor.h"

#include <memory>
#include <string>
#include <vector>

class World
{
public:
	World();
	~World();

	World(const World&) = delete;
	World& operator=(const World&) = delete;
	World(World&&) = delete;
	World& operator=(World&&) = delete;

	Actor* CreateActor(std::string aName);
	Actor* FindActor(const std::string& aName) const;

	void Update(float aDeltaTime);

	const std::vector<std::unique_ptr<Actor>>& GetActors() const;

private:
	bool CanAddActorName(const std::string& aName) const;
	void ReportDuplicateActorName(const std::string& aName) const;

	std::vector<std::unique_ptr<Actor>> myActors;
};
