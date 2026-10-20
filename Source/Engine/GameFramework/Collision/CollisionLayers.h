#pragma once

// Edit these names for the game. Each value selects one of the 32 layer bits.
enum class CollisionLayer : unsigned int
{
	Default = 0,
	Player = 1,
	Trigger = 2,
	Enemy = 3
};
