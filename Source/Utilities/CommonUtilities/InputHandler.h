#pragma once

#include "Vector2.hpp"
#include "EnumLockMode.h"

#include <bitset>
#include <Windows.h>

namespace CommonUtilities
{
	class InputHandler
	{
	public:
		InputHandler();
		explicit InputHandler(HWND aWindowHandle);
		~InputHandler() = default;

		bool UpdateEvents(UINT aMessage, WPARAM wParam, LPARAM lParam);
		void UpdateInput();

		bool IsKeyDown(int aKeyCode) const;
		bool IsKeyPressed(int aKeyCode) const;
		bool IsKeyReleased(int aKeyCode) const;
		bool IsMouseMoved() const;

		Vector2<int> GetMousePosition() const;
		Vector2<int> GetMouseDelta() const;

		void SetWindowHandle(HWND aWindowHandle);
		HWND GetWindowHandle() const;

		void SetCursorVisibility(bool aShow);
		void SetCursorLockMode(LockMode aLockMode);
		void CenterMouse();

	private:
		void UpdateWindowAreaEvent();
		void SetLockArea() const;

		std::bitset<256> myCurrentState;
		std::bitset<256> myPreviousState;
		std::bitset<256> myTentativeState;

		Vector2<int> myCurrentMousePosition;
		Vector2<int> myTentativeMousePosition;
		Vector2<int> myCurrentMouseDelta;
		Vector2<int> myTentativeMouseDelta;

		RECT myWindowArea = {};
		RECT myLockedArea = {};

		HWND myWindowHandle = nullptr;

		LockMode myLockMode = LockMode::None;
	};
}
