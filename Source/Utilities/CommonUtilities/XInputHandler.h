#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "Vector2.hpp"
#include <windows.h>
#include <Xinput.h>

namespace CommonUtilities
{
	class XInputHandler
	{
	public:
		XInputHandler();
		~XInputHandler() = default;

		bool UpdateInput();
		bool IsConnected(int aGamepadIndex) const;
		bool TryReconnect();

		bool IsButtonDown(unsigned aGamepadCode) const;
		bool IsButtonPressed(unsigned aGamepadCode) const;
		bool IsButtonReleased(unsigned aGamepadCode) const;

		float GetTriggerLeftValue() const;
		float GetTriggerRightValue() const;

		bool GetAnalogLeftValue(Vector2<float>& outAnalogValue) const;
		bool GetAnalogRightValue(Vector2<float>& outAnalogValue) const;

	private:
		XINPUT_STATE myInputState;
		uint16_t myPreviousGamepadState;
		int myGamepadIndex;

		bool myPreviousLTriggerWasNonZero;
		bool myPreviousRTriggerWasNonZero;
		bool myPreviousLThumbWasNonZero;
		bool myPreviousRThumbWasNonZero;

		bool GetTransformedAnalogValues(const SHORT aThumbXValue, const SHORT aThumbYValue, const int aThreshold, Vector2<float>& outAnalogValues) const;
		float GetTransformedRangeValue(const BYTE aTriggerValue, const int aThreshold) const;
	};
}
