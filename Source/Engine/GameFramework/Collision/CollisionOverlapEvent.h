#pragma once
#include <cstdint>

class World;
class Actor;
class ColliderComponent;
using ColliderId = std::uint64_t;
enum class CollisionOverlapPhase
{
	Enter,
	Stay,
	Exit
};

// Collision details for one collider's callback.
// Use these details during the callback; do not store Actor or collider pointers for later frames.
struct Collision
{
	World* SourceWorld = nullptr;
	// The number still identifies the other collider even if that collider has been destroyed.
	ColliderId OtherColliderId = 0;
	CollisionOverlapPhase Phase = CollisionOverlapPhase::Enter;

	// These return nullptr if the other participant has been destroyed.
	ColliderComponent* OtherCollider() const;
	Actor* OtherActor() const;
};

// IDs survive participant destruction; lookup during Exit may return null.
// SourceWorld is borrowed for synchronous dispatch. IDs belong only to that World.
struct CollisionOverlapEvent
{
	World* SourceWorld = nullptr;
	ColliderId FirstColliderId = 0;
	ColliderId SecondColliderId = 0;
	CollisionOverlapPhase Phase = CollisionOverlapPhase::Enter;
};
