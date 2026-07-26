#include <AGP/RendererHost.h>

#include <Windows.h>

#include <array>
#include <filesystem>
#include <iostream>
#include <string_view>

namespace
{
	LRESULT CALLBACK TestWindowProc(HWND aWindow, UINT aMessage, WPARAM aWParam, LPARAM aLParam)
	{
		return DefWindowProcW(aWindow, aMessage, aWParam, aLParam);
	}

	int fail(const char* aMessage)
	{
		std::cerr << aMessage << '\n';
		return 1;
	}
}

int wmain(int aArgumentCount, wchar_t** aArguments)
{
	if (aArgumentCount != 2)
	{
		return fail("Expected the staged shader-root path.");
	}
	const std::filesystem::path shaderRoot = aArguments[1];
	const std::filesystem::path environmentTexture = shaderRoot.parent_path() / "fixtures" / "T_Shipyard.dds";
	const AGP::RendererHostResult invalidInitialization =
		AGP::InitializeRendererHost(nullptr, shaderRoot.c_str(), environmentTexture.c_str());
	if (invalidInitialization.Status != AGP::RendererHostStatus::InvalidArgument
		|| std::string_view(invalidInitialization.Code) != "renderer.invalid_argument")
	{
		return fail("Renderer host accepted a null native window.");
	}

	const HINSTANCE instance = GetModuleHandleW(nullptr);
	const wchar_t* className = L"AGPRendererHostBundleTest";
	WNDCLASSW windowClass = {};
	windowClass.lpfnWndProc = TestWindowProc;
	windowClass.hInstance = instance;
	windowClass.lpszClassName = className;
	if (RegisterClassW(&windowClass) == 0)
	{
		return fail("Could not register the renderer-host test window class.");
	}

	const HWND window = CreateWindowExW(
		0, className, L"AGP Renderer Host Bundle Test", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 640, 360, nullptr, nullptr, instance, nullptr);
	if (window == nullptr)
	{
		UnregisterClassW(className, instance);
		return fail("Could not create the renderer-host test window.");
	}

	int result = 0;
	if (!AGP::InitializeRendererHost(window, shaderRoot.c_str(), environmentTexture.c_str()).Succeeded())
	{
		result = fail("Could not initialize AGP through the staged renderer-host contract.");
	}
	else
	{
		const AGP::NativeD3D11View nativeView = AGP::GetRendererHostNativeD3D11View();
		if (nativeView.Device == nullptr || nativeView.ImmediateContext == nullptr)
		{
			result = fail("The initialized renderer host returned an incomplete D3D11 view.");
		}
		else if (!AGP::BeginRendererHostFrame(std::array{ 0.05f, 0.07f, 0.10f, 1.0f }).Succeeded())
		{
			result = fail("The renderer host could not begin a backbuffer frame.");
		}
		else if (!AGP::ResizeRendererHost(800, 450).Succeeded()
			|| !AGP::BeginRendererHostFrame(std::array{ 0.08f, 0.10f, 0.14f, 1.0f }).Succeeded())
		{
			result = fail("The renderer host could not resize and begin another frame.");
		}
		else if (!AGP::PresentRendererHostFrame().Succeeded())
		{
			result = fail("The renderer host could not present its frame.");
		}
	}

	DestroyWindow(window);
	UnregisterClassW(className, instance);
	return result;
}
