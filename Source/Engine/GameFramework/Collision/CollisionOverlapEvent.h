#pragma once
#include <cstdint>

class World;
class Actor;
class ColliderComponent;
using ColliderId = std::uint64_t;
enum class CollisionOverlapPhase
{
	// First overlapping frame, later overlapping frames, then the first frame apart.
	Enter,
	Stay,
	Exit
};

struct Collision
{
	World* SourceWorld = nullptr;

	ColliderId OtherColliderId = 0;
	CollisionOverlapPhase Phase = CollisionOverlapPhase::Enter;

	ColliderComponent* OtherCollider() const;
	Actor* OtherActor() const;
};

struct CollisionOverlapEvent
{
	World* SourceWorld = nullptr;
	ColliderId FirstColliderId = 0;
	ColliderId SecondColliderId = 0;
	CollisionOverlapPhase Phase = CollisionOverlapPhase::Enter;
};
