#include "XInputHandler.h"
#include <limits>
#pragma comment(lib, "Xinput9_1_0.lib")

namespace CommonUtilities
{
	XInputHandler::XInputHandler()
		: myInputState({})
		, myPreviousGamepadState(0u)
		, myGamepadIndex(-1)
		, myPreviousLTriggerWasNonZero(false)
		, myPreviousRTriggerWasNonZero(false)
		, myPreviousLThumbWasNonZero(false)
		, myPreviousRThumbWasNonZero(false)
	{}

	bool XInputHandler::UpdateInput()
	{
		if (myGamepadIndex == -1)
		{
			TryReconnect();

			if (myGamepadIndex == -1)
			{
				myPreviousGamepadState = 0;
				return false;
			}
		}

		myPreviousGamepadState = myInputState.Gamepad.wButtons;
		ZeroMemory(&myInputState, sizeof(XINPUT_STATE));

		if (XInputGetState(myGamepadIndex, &myInputState) == ERROR_SUCCESS)
		{
			return true;
		}

		myGamepadIndex = -1;
		return false;
	}

	bool XInputHandler::IsConnected(int aGamepadIndex) const
	{
		XINPUT_STATE inputState;
		ZeroMemory(&inputState, sizeof(XINPUT_STATE));

		return XInputGetState(aGamepadIndex, &inputState) == ERROR_SUCCESS;
	}

	bool XInputHandler::TryReconnect()
	{
		for (int i = 0; i < XUSER_MAX_COUNT; ++i)
		{
			XINPUT_STATE inputState;
			ZeroMemory(&inputState, sizeof(XINPUT_STATE));

			if (XInputGetState(i, &inputState) == ERROR_SUCCESS)
			{
				myGamepadIndex = i;
				return true;
			}
		}

		return false;
	}

	bool XInputHandler::IsButtonDown(unsigned aGamepadCode) const
	{
		return (myInputState.Gamepad.wButtons & aGamepadCode) != 0u;
	}

	bool XInputHandler::IsButtonPressed(unsigned aGamepadCode) const
	{
		return ((myInputState.Gamepad.wButtons ^ myPreviousGamepadState) & myInputState.Gamepad.wButtons & aGamepadCode) != 0u;
	}

	bool XInputHandler::IsButtonReleased(unsigned aGamepadCode) const
	{
		return ((myInputState.Gamepad.wButtons ^ myPreviousGamepadState) & myPreviousGamepadState & aGamepadCode) != 0u;
	}

	float XInputHandler::GetTriggerLeftValue() const
	{
		return GetTransformedRangeValue(myInputState.Gamepad.bLeftTrigger, XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
	}

	float XInputHandler::GetTriggerRightValue() const
	{
		return GetTransformedRangeValue(myInputState.Gamepad.bRightTrigger, XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
	}

	bool XInputHandler::GetAnalogLeftValue(Vector2<float>& outAnalogValue) const
	{
		return GetTransformedAnalogValues(myInputState.Gamepad.sThumbLX, myInputState.Gamepad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, outAnalogValue);
	}

	bool XInputHandler::GetAnalogRightValue(Vector2<float>& outAnalogValue) const
	{
		return GetTransformedAnalogValues(myInputState.Gamepad.sThumbRX, myInputState.Gamepad.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE, outAnalogValue);
	}

	bool XInputHandler::GetTransformedAnalogValues(const SHORT aThumbXValue, const SHORT aThumbYValue, const int aThreshold, Vector2<float>& outAnalogValues) const
	{
		const SHORT absThumbX = static_cast<SHORT>(std::abs(aThumbXValue));
		const SHORT absThumbY = static_cast<SHORT>(std::abs(aThumbYValue));
		const float invMax = 1.0f / static_cast<float>(std::numeric_limits<short>::max() - aThreshold);
		bool isEngaged = false;

		if (absThumbX > aThreshold)
		{
			const int rawX = absThumbX - aThreshold;
			const float signX = aThumbXValue < 0 ? -1.0f : 1.0f;
			outAnalogValues.x = static_cast<float>(rawX) * signX * invMax;
			isEngaged = true;
		}

		if (absThumbY > aThreshold)
		{
			const int rawY = absThumbY - aThreshold;
			const float signY = aThumbYValue < 0 ? -1.0f : 1.0f;
			outAnalogValues.y = static_cast<float>(rawY) * signY * invMax;
			isEngaged = true;
		}

		return isEngaged;
	}

	float XInputHandler::GetTransformedRangeValue(const BYTE aTriggerValue, const int aThreshold) const
	{
		if (aTriggerValue > aThreshold)
		{
			return static_cast<float>(aTriggerValue - aThreshold) / static_cast<float>(std::numeric_limits<BYTE>::max() - aThreshold);
		}

		return 0.0f;
	}
}
