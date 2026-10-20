#pragma once
#include "GameFramework/Collision/CollisionOverlapEvent.h"
#include <set>
#include <utility>
#include <vector>

class Component;
class ColliderComponent;

class CollisionSystem
{
public:
	void RegisterCollider(ColliderComponent& collider);
	void Update(World& world, const std::vector<Component*>& frame);

	// Called by World::Clear to forget overlaps without sending Exit events.
	void Clear()
	{
		myPairs.clear();
	}

private:
	ColliderId myNextColliderId = 1;
	std::set<std::pair<ColliderId, ColliderId>> myPairs;
};
