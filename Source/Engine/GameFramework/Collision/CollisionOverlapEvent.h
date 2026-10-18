#pragma once
#include <cstdint>

class World;
using ColliderId = std::uint64_t;
enum class CollisionOverlapPhase
{
	Enter,
	Stay,
	Exit
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
