#include "InputHandler.h"
#include "EnumKeyCode.h"
#include "Vector2.hpp"
#include <WinUser.h>
#include <iostream>

namespace CommonUtilities
{
	InputHandler::InputHandler() = default;

	InputHandler::InputHandler(HWND aWindowHandle) : myWindowHandle(aWindowHandle) {}

	bool InputHandler::UpdateEvents(UINT aMessage, WPARAM wParam, LPARAM lParam)
	{
		switch (aMessage)
		{
			case WM_KEYDOWN:
				myTentativeState.set(wParam);
				return true;

			case WM_KEYUP:
				myTentativeState.reset(wParam);
				return true;

			case WM_LBUTTONDOWN:
				myTentativeState.set(static_cast<unsigned>(EKeyCode::MOUSELBUTTON));
				return true;

			case WM_RBUTTONDOWN:
				myTentativeState.set(static_cast<unsigned>(EKeyCode::MOUSERBUTTON));
				return true;

			case WM_MBUTTONDOWN:
				myTentativeState.set(static_cast<unsigned>(EKeyCode::MBUTTON));
				return true;

			case WM_LBUTTONUP:
				myTentativeState.reset(static_cast<unsigned>(EKeyCode::MOUSELBUTTON));
				return true;

			case WM_RBUTTONUP:
				myTentativeState.reset(static_cast<unsigned>(EKeyCode::MOUSERBUTTON));
				return true;

			case WM_MBUTTONUP:
				myTentativeState.reset(static_cast<unsigned>(EKeyCode::MBUTTON));
				return true;

			case WM_MOUSEMOVE:
			{
				const Vector2<int> newPosition
				{
					static_cast<int>(lParam & 0xffff),
					static_cast<int>((lParam >> 16) & 0xffff)
				};

				myTentativeMouseDelta += newPosition - myTentativeMousePosition;
				myTentativeMousePosition = newPosition;
				return true;
			}

			default: return false;
		}
	}

	void InputHandler::UpdateInput()
	{
		myPreviousState = myCurrentState;
		myCurrentState = myTentativeState;
		myCurrentMousePosition = myTentativeMousePosition;

		myCurrentMouseDelta = myTentativeMouseDelta;
		myTentativeMouseDelta.x = 0;
		myTentativeMouseDelta.y = 0;
	}

	bool InputHandler::IsKeyDown(const int aKeyCode) const
	{
		return myCurrentState.test(static_cast<size_t>(aKeyCode));
	}

	bool InputHandler::IsKeyPressed(const int aKeyCode) const
	{
		if (myCurrentState.test(static_cast<size_t>(aKeyCode)))
		{
			if (!myPreviousState.test(static_cast<size_t>(aKeyCode)))
			{
				return true;
			}
		}

		return false;
	}

	bool InputHandler::IsKeyReleased(const int aKeyCode) const
	{
		if (!myCurrentState.test(static_cast<size_t>(aKeyCode)))
		{
			if (myPreviousState.test(static_cast<size_t>(aKeyCode)))
			{
				return true;
			}
		}

		return false;
	}

	bool InputHandler::IsMouseMoved() const
	{
		return (myCurrentMouseDelta.x != 0 || myCurrentMouseDelta.y != 0);
	}

	Vector2<int> InputHandler::GetMousePosition() const
	{
		return myCurrentMousePosition;
	}

	Vector2<int> InputHandler::GetMouseDelta() const
	{
		return myCurrentMouseDelta;
	}

	HWND InputHandler::GetWindowHandle() const
	{
		return myWindowHandle;
	}

	void InputHandler::SetWindowHandle(HWND aWindowHandle)
	{
		myWindowHandle = aWindowHandle;
	}

	void InputHandler::SetAutoMouseCapture(bool anEnabled)
	{
		myAutoMouseCapture = anEnabled;
	}

	void InputHandler::ShowCursor()
	{
		if (!myIsCursorVisible)
		{
			::ShowCursor(true);
			myIsCursorVisible = true;
		}
	}

	void InputHandler::HideCursor()
	{
		if (myIsCursorVisible)
		{
			::ShowCursor(false);
			myIsCursorVisible = false;
		}
	}

	void InputHandler::CaptureMouse()
	{
		if (myIsMouseCaptured)
		{
			return;
		}

		RECT clipRect;

		GetClientRect(myWindowHandle, &clipRect);

		POINT rectMin
		{
			clipRect.left,
			clipRect.top
		};

		POINT rectMax
		{
			clipRect.right,
			clipRect.bottom
		};

		MapWindowPoints(myWindowHandle, nullptr, &rectMin, 1);
		MapWindowPoints(myWindowHandle, nullptr, &rectMax, 1);

		clipRect.left = rectMin.x;
		clipRect.top = rectMin.y;
		clipRect.right = rectMax.x;
		clipRect.bottom = rectMax.y;

		ClipCursor(&clipRect);
		myIsMouseCaptured = true;
	}

	void InputHandler::ReleaseMouse()
	{
		if (myIsMouseCaptured)
		{
			ClipCursor(nullptr);
			myIsMouseCaptured = false;
		}
	}

	void InputHandler::CenterMouse()
	{
		RECT clientRect;
		GetClientRect(myWindowHandle, &clientRect);

		POINT center
		{
			(clientRect.right - clientRect.left) / 2,
			(clientRect.bottom - clientRect.top) / 2
		};

		myTentativeMousePosition.x = center.x;
		myTentativeMousePosition.y = center.y;

		ClientToScreen(myWindowHandle, &center);
		SetCursorPos(center.x, center.y);
	}
}
