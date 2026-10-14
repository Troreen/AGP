#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

namespace GameWindowMessages
{
	LRESULT CALLBACK WindowProc(HWND aWindow, UINT aMessage, WPARAM aWParam, LPARAM anLParam);
	void Pump(bool& aQuitRequested);
}
