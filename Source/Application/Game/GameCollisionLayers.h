#pragma once
#include <string>
#include <unordered_map>

namespace GameCollisionLayers
{
	inline constexpr unsigned int Player = 1;
	inline constexpr unsigned int Trigger = 2;
	inline constexpr unsigned int Enemy = 3;

	inline std::unordered_map<std::string, unsigned int> Names()
	{
		return {{"Player", Player}, {"Trigger", Trigger}, {"Enemy", Enemy}};
	}
}
