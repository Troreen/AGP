#include "GameWindowMessages.h"

#include "InputHandler.h"
#include "imgui.h"
#include "imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace
{
#ifdef _DEBUG
	bool debugUiVisible = true;
#else
	bool debugUiVisible = false;
#endif
}

bool GameWindowMessages::IsDebugUiVisible()
{
	return debugUiVisible;
}

void GameWindowMessages::ToggleDebugUiVisibility()
{
	debugUiVisible = !debugUiVisible;
}

LRESULT CALLBACK GameWindowMessages::WindowProc(HWND aWindow, UINT aMessage, WPARAM aWParam, LPARAM anLParam)
{
	if (aMessage == WM_NCCREATE)
	{
		const auto* create = reinterpret_cast<const CREATESTRUCTW*>(anLParam);
		SetWindowLongPtrW(aWindow, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
	}
	auto* input = reinterpret_cast<CommonUtilities::InputHandler*>(GetWindowLongPtrW(aWindow, GWLP_USERDATA));
	if (aMessage == WM_KEYDOWN && aWParam == VK_F9 && (anLParam & (1LL << 30)) == 0)
	{
		ToggleDebugUiVisibility();
		if (input)
		{
			input->ClearInputState();
		}
		return 0;
	}
	if ((aMessage == WM_KEYDOWN || aMessage == WM_KEYUP) && aWParam == VK_F9)
	{
		return 0;
	}
	if (ImGui::GetCurrentContext())
	{
		const LRESULT imguiResult = ImGui_ImplWin32_WndProcHandler(aWindow, aMessage, aWParam, anLParam);
		if (debugUiVisible && imguiResult != 0)
		{
			return imguiResult;
		}
		const ImGuiIO& io = ImGui::GetIO();
		const bool mouseMessage = (aMessage >= WM_MOUSEFIRST && aMessage <= WM_MOUSELAST);
		const bool keyboardMessage = aMessage == WM_KEYDOWN || aMessage == WM_KEYUP ||
			aMessage == WM_SYSKEYDOWN || aMessage == WM_SYSKEYUP;
		if (debugUiVisible && ((mouseMessage && io.WantCaptureMouse) || (keyboardMessage && io.WantCaptureKeyboard)))
		{
			if (input)
			{
				if (aMessage == WM_MOUSEMOVE)
				{
					input->UpdateEvents(aMessage, aWParam, anLParam);
				}
				input->ClearInputState();
			}
			return 0;
		}
	}
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
