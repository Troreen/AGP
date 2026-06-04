#pragma once

#include <Windows.h>
#include <Windef.h>
#include <bitset>
#include "EnumKeys.h"

namespace CommonUtilities
{
	class InputHandler
	{
	public:
		InputHandler();
		~InputHandler();

		bool UpdateEvents(UINT message, WPARAM wParam, LPARAM lParam);
		bool UpdateMouseInput(UINT aMessage, WPARAM wParam, LPARAM lParam);

		void UpdateInput();

		bool IsKeyDown(const int aKeyCode) const;
		bool IsKeyPressed(const int aKeyCode) const;
		bool IsKeyReleased(const int aKeyCode) const;

		bool IsKeyDown(Keys key) const;
		bool IsKeyPressed(Keys key) const;
		bool IsKeyReleased(Keys key) const;

		POINT GetMousePos() const;
		POINT GetMouseDelta() const;
		POINT GetMousePosScreen() const;
		int GetWheelDeltaX() const;
		int GetWheelDeltaY() const;

		bool IsMouseButtonDown(int buttonVk) const;
		bool IsMouseButtonPressed(int buttonVk) const;
		bool IsMouseButtonReleased(int buttonVk) const;

		bool IsMouseButtonDown(Keys key) const;
		bool IsMouseButtonPressed(Keys key) const;
		bool IsMouseButtonReleased(Keys key) const;

		void SetWindowHandle(HWND hWnd);
		HWND GetWindowHandle() const;

		void SetAutoMouseCapture(bool enabled);
		bool GetAutoMouseCapture() const; 
		bool IsMouseCaptured() const;	

	private:
		std::bitset<256> myCurrentState;
		std::bitset<256> myPreviousState;
		std::bitset<256> myTentativeState;

		POINT myCurrentMousePos;
		POINT myPreviousMousePos;
		POINT myTentativeMousePos;

		int myWheelX;
		int myWheelY;
		int myTentativeWheelX;
		int myTentativeWheelY;

		HWND myHwnd;
		bool myAutoMouseCapture;
		bool myMouseCaptured;

		bool AnyMouseButtonDownTentative() const;
		void TryBeginMouseCapture();
		void TryEndMouseCaptureIfNoButtons();
	};
} // namespace CommonUtilities