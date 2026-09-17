#pragma once
#include <array>
#include <cstddef>
#include "EnumKeys.h"

// Input sampled once per frame. Pressed is for toggles; Down is for held actions.
class GameInput
{
public:
	std::array<bool, 256> KeysDown = {};
	std::array<bool, 256> KeysPressed = {};
	bool MouseLookActive = false;
	float MouseDeltaX = 0;
	float MouseDeltaY = 0;

	bool IsKeyDown(Keys key) const
	{
		const auto i = static_cast<size_t>(key);
		return i < KeysDown.size() && KeysDown[i];
	}

	bool IsKeyPressed(Keys key) const
	{
		const auto i = static_cast<size_t>(key);
		return i < KeysPressed.size() && KeysPressed[i];
	}
};
