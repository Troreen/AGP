#pragma once
#include <array>
#include <cstddef>
#include "EnumKeys.h"

namespace GameFrameworkInternal { class InputAccess; }

// Stable input for a callback. Edges survive coalescing of platform frames.
// Game code should query IsKeyDown for continuous actions and IsKeyPressed for
// one-shot actions. Choose either the fixed or variable phase for an action to
// avoid handling the same press in both. This is a sampled state, not an event log:
// multiple presses of one key within a combined sample collapse to one edge.
struct GameInput
{
	std::array<bool, 256> KeysDown = {};
	std::array<bool, 256> KeysPressed = {};
	bool MouseLookActive = false;
	float MouseDeltaX = 0;
	float MouseDeltaY = 0;

	bool IsKeyDown(Keys key) const { const auto i = static_cast<size_t>(key); return i < KeysDown.size() && KeysDown[i]; }
	bool IsKeyPressed(Keys key) const { const auto i = static_cast<size_t>(key); return i < KeysPressed.size() && KeysPressed[i]; }
private:
	// Engine bookkeeping: consume transient presses/motion without releasing held keys.
	void ClearPressed() { KeysPressed.fill(false); MouseDeltaX = MouseDeltaY = 0; }
	// Engine bookkeeping: newest held state wins, but preserve pending presses and
	// sum motion while the consumer is behind. Preserve accumulated motion even when
	// mouse-look was released in the newest sample.
	void Merge(const GameInput& newer)
	{
		KeysDown = newer.KeysDown;
		for (size_t i = 0; i < KeysPressed.size(); ++i) KeysPressed[i] = KeysPressed[i] || newer.KeysPressed[i];
		MouseDeltaX += newer.MouseDeltaX;
		MouseDeltaY += newer.MouseDeltaY;
		MouseLookActive = newer.MouseLookActive || MouseDeltaX != 0 || MouseDeltaY != 0;
	}
    friend class GameFrameworkInternal::InputAccess;
};
