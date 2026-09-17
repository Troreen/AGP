#pragma once
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include "InputAccess.h"

namespace GameFrameworkInternal
{
	// Shared by threaded and synchronous execution, independent of platform/render APIs.
	// Engine-only timing policy. This class orders phases; it does not own a World,
	// spawn a thread, or call rendering. Keeping it platform-independent lets tests
	// exercise the same timing used by threaded and synchronous execution.
	class GameLoop
	{
	public:
		explicit GameLoop(float fixedDelta) : myFixedDelta(fixedDelta)
		{
			if (!std::isfinite(fixedDelta) || fixedDelta <= 0) throw std::invalid_argument("FixedDeltaTime must be positive and finite");
		}
		void Reset() { myAccumulator = 0; myFixedInput = {}; }
        // Advance one gameplay frame, which may combine several platform frames.
		// A fixed step is not guaranteed on every call; variable Update and Late always run.
		template<class Fixed, class Update, class Late>
		void Advance(float delta, const GameInput& input, Fixed fixed, Update update, Late late)
		{
			delta = std::isfinite(delta) ? std::clamp(delta, 0.0f, 0.25f) : 0.0f;
			// Retain fixed-phase input across frames with no fixed step. A press must reach
			// the next fixed tick even if variable Update has already observed it.
			InputAccess::Merge(myFixedInput, input);
			myAccumulator += delta;
			int steps = 0;
			// Bound catch-up work so a slow frame cannot create an endless simulation backlog.
			// Transient fixed input clears after the first tick; held keys survive all ticks.
			while (myAccumulator >= myFixedDelta && steps < 5)
			{
				fixed(myFixedDelta, myFixedInput);
				InputAccess::ClearPressed(myFixedInput);
				myAccumulator -= myFixedDelta;
				++steps;
			}
			// Drop excess whole steps but preserve the fractional remainder. Under overload
			// simulation deliberately loses time; this is not a deterministic replay clock.
			if (steps == 5 && myAccumulator >= myFixedDelta)
				myAccumulator = std::fmod(myAccumulator, myFixedDelta);
			// These phases see the original frame input, independently of fixed consumption.
			update(delta, input);
			late(delta, input);
		}
	private:
		float myFixedDelta;
		float myAccumulator = 0;
		GameInput myFixedInput;
	};
}
