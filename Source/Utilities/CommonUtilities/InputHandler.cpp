#include "InputHandler.h"

#include "EnumKeyCode.h"
#include "Vector2.hpp"

namespace CommonUtilities
{
	InputHandler::InputHandler() = default;

	InputHandler::InputHandler(HWND aWindowHandle) : myWindowHandle(aWindowHandle) {}

	bool InputHandler::UpdateEvents(UINT aMessage, WPARAM wParam, LPARAM lParam)
	{
		switch (aMessage)
		{
			case WM_KILLFOCUS:
				// Let the mapper emit releases on its next (single) frame update.
				myTentativeState.reset();
				myTentativeMouseDelta = {};
				SetCursorLockMode(LockMode::None);
				return true;

			case WM_KEYDOWN:
			case WM_SYSKEYDOWN:
				myTentativeState.set(wParam);
				return true;

			case WM_KEYUP:
			case WM_SYSKEYUP:
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
					static_cast<short>(lParam & 0xffff),
					static_cast<short>((lParam >> 16) & 0xffff)
				};

				myTentativeMouseDelta += newPosition - myTentativeMousePosition;
				myTentativeMousePosition = newPosition;
				return true;
			}

			case WM_ACTIVATEAPP:
			{
				if (wParam)
				{
					UpdateWindowAreaEvent();
					SetLockArea();
				}
				else
				{
					ClipCursor(NULL);
				}
				return true;
			}

			case WM_SIZE:
			case WM_MOVE:
			{
				UpdateWindowAreaEvent();

				if (GetForegroundWindow() == myWindowHandle)
				{
					SetLockArea();
				}
				return true;
			}

			default:
				return false;
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
		return myCurrentState[aKeyCode];
	}

	bool InputHandler::IsKeyPressed(const int aKeyCode) const
	{
		return myCurrentState[aKeyCode] && !myPreviousState[aKeyCode];
	}

	bool InputHandler::IsKeyReleased(const int aKeyCode) const
	{
		return !myCurrentState[aKeyCode] && myPreviousState[aKeyCode];
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

	void InputHandler::SetCursorVisibility(bool aShow)
	{
		for (int i = 0; i < 100; ++i)
		{
			const int result = ShowCursor(aShow);
			if ((aShow && result >= 0) || (!aShow && result < 0))
			{
				break;
			}
		}
	}

	void InputHandler::SetCursorLockMode(LockMode aLockMode)
	{
		myLockMode = aLockMode;
		SetLockArea();
	}

	void InputHandler::CenterMouse()
	{
		RECT clientRect{};
		if (!myWindowHandle || !GetClientRect(myWindowHandle, &clientRect))
		{
			return;
		}

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

	void InputHandler::UpdateWindowAreaEvent()
	{
		if (!GetClientRect(myWindowHandle, &myWindowArea))
		{
			return;
		}

		myLockedArea.left = static_cast<LONG>(static_cast<double>(myWindowArea.left) + (myWindowArea.right * 0.5));
		myLockedArea.top = static_cast<LONG>(static_cast<double>(myWindowArea.top) + (myWindowArea.bottom * 0.5));
		myLockedArea.right = myLockedArea.left + 1;
		myLockedArea.bottom = myLockedArea.top + 1;

		MapWindowPoints(myWindowHandle, NULL, (LPPOINT)&myWindowArea, 2);
		MapWindowPoints(myWindowHandle, NULL, (LPPOINT)&myLockedArea, 2);
	}

	void InputHandler::SetLockArea() const
	{
		switch (myLockMode)
		{
			case LockMode::None:
				ClipCursor(NULL);
				break;

			case LockMode::Restricted:
				ClipCursor(&myWindowArea);
				break;

			case LockMode::Locked:
				ClipCursor(&myLockedArea);
				break;
		}
	}
}
