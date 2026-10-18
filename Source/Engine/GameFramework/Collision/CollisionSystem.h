#pragma once
#include "GameFramework/Collision/CollisionOverlapEvent.h"
#include <set>
#include <utility>
#include <vector>

class Component;

// Owned by World. Pair state stores identities, never borrowed component pointers.
class CollisionSystem
{
public:
	void Update(World& world, const std::vector<Component*>& frame);

	void Clear()
	{
		myPairs.clear();
	}

private:
	std::set<std::pair<ColliderId, ColliderId>> myPairs;
};
