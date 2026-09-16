#pragma once

#include "Actor.h"

#include <memory>
#include <string>
#include <vector>

// Owns the session actor collection and dispatches component phases. Game code
// creates actors here during initialization or top-level game callbacks. The current
// world has no deferred mutation, actor destruction, or scene-replacement service.
class World
{
public:
	World();
	~World();

	World(const World&) = delete;
	World& operator=(const World&) = delete;
	World(World&&) = delete;
	World& operator=(World&&) = delete;

	// Names must be non-empty and unique when creating actors. Results are borrowed
	// pointers, not stable serialized IDs; a future scene loader needs explicit IDs.
	Actor* CreateActor(std::string aName);
	Actor* FindActor(const std::string& aName) const;

	// Host-owned ticking. Update includes both the Update and LateUpdate passes.
	// Do not invoke these manually from IGame or components: it would double-tick state.
	void FixedUpdate(float aDeltaTime);
	void Update(float aDeltaTime);

	const std::vector<std::unique_ptr<Actor>>& GetActors() const;

private:
	bool CanAddActorName(const std::string& aName) const;
	void ReportDuplicateActorName(const std::string& aName) const;

	std::vector<std::unique_ptr<Actor>> myActors;
};
