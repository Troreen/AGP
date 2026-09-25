#include "GameWindowMessages.h"

#include "InputHandler.h"

LRESULT CALLBACK GameWindowMessages::WindowProc(HWND aWindow, UINT aMessage, WPARAM aWParam, LPARAM anLParam)
{
	if (aMessage == WM_NCCREATE)
	{
		const auto* create = reinterpret_cast<const CREATESTRUCTW*>(anLParam);
		SetWindowLongPtrW(aWindow, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
	}
	auto* input = reinterpret_cast<CommonUtilities::InputHandler*>(GetWindowLongPtrW(aWindow, GWLP_USERDATA));
	if (input)
	{
		input->UpdateEvents(aMessage, aWParam, anLParam);
	}
	if (aMessage == WM_CLOSE || aMessage == WM_DESTROY)
	{
		PostQuitMessage(0);
	}
	return DefWindowProcW(aWindow, aMessage, aWParam, anLParam);
}

void GameWindowMessages::Pump(bool& aQuitRequested)
{
	MSG message{};
	while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
	{
		if (message.message == WM_QUIT)
		{
			aQuitRequested = true;
		}
		TranslateMessage(&message);
		DispatchMessageW(&message);
	}
}
