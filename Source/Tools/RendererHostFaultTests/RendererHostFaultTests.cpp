#include "GraphicsEngine/RendererHost.h"
#include "GraphicsEngine/RendererHostFaultInjection.h"

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

	int Fail(const char* aMessage)
	{
		std::cerr << aMessage << '\n';
		return 1;
	}
}

int wmain(int aArgumentCount, wchar_t** aArguments)
{
	if (aArgumentCount != 3)
	{
		return Fail("Expected the shader-root path and a fault-test mode.");
	}

	const std::filesystem::path shaderRoot = aArguments[1];
	const std::filesystem::path environmentTexture = shaderRoot.parent_path() / "fixtures" / "T_Shipyard.dds";
	const std::wstring_view mode = aArguments[2];
	const HINSTANCE instance = GetModuleHandleW(nullptr);
	const wchar_t* className = L"AGPRendererHostFaultTest";
	WNDCLASSW windowClass = {};
	windowClass.lpfnWndProc = TestWindowProc;
	windowClass.hInstance = instance;
	windowClass.lpszClassName = className;
	if (RegisterClassW(&windowClass) == 0)
	{
		return Fail("Could not register the fault-test window class.");
	}

	const HWND window = CreateWindowExW(
		0, className, L"AGP Renderer Host Fault Test", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 640, 360, nullptr, nullptr, instance, nullptr);
	if (window == nullptr)
	{
		UnregisterClassW(className, instance);
		return Fail("Could not create the fault-test window.");
	}

	int result = 0;
	if (mode == L"partial-init-retry")
	{
		AGP::Testing::SetRendererHostFault(AGP::Testing::RendererHostFault::AfterGraphicsInitialize);
		const AGP::RendererHostResult partialFailure = AGP::InitializeRendererHost(window, shaderRoot.c_str(), environmentTexture.c_str());
		const AGP::RendererHostResult retry = AGP::InitializeRendererHost(window, shaderRoot.c_str(), environmentTexture.c_str());
		if (partialFailure.Status != AGP::RendererHostStatus::InitializationFailed
			|| std::string_view(partialFailure.Code) != "renderer.initialization_injected_failure"
			|| retry.Status != AGP::RendererHostStatus::RendererUnavailable
			|| std::string_view(retry.Code) != "renderer.restart_required"
			|| AGP::GetRendererHostNativeD3D11View().Device != nullptr)
		{
			result = Fail("Partial initialization did not reject retry and hide resources.");
		}
	}
	else if (mode == L"resize-recovery")
	{
		if (!AGP::InitializeRendererHost(window, shaderRoot.c_str(), environmentTexture.c_str()).Succeeded())
		{
			result = Fail("Could not initialize the fault-test renderer.");
		}
		else
		{
			AGP::Testing::SetRendererHostFault(AGP::Testing::RendererHostFault::AfterSwapchainResize);
			const AGP::RendererHostResult injectedResize = AGP::ResizeRendererHost(800, 450);
			if (injectedResize.Status != AGP::RendererHostStatus::RendererUnavailable
				|| std::string_view(injectedResize.Code) != "renderer.resize_recovery_required"
				|| std::string_view(AGP::BeginRendererHostFrame(std::array{ 0.0f, 0.0f, 0.0f, 1.0f }).Code) != "renderer.resize_recovery_required"
				|| std::string_view(AGP::PresentRendererHostFrame().Code) != "renderer.resize_recovery_required")
			{
				result = Fail("Post-swapchain failure did not block frame submission.");
			}
			else if (!AGP::ResizeRendererHost(800, 450).Succeeded()
				|| !AGP::BeginRendererHostFrame(std::array{ 0.08f, 0.10f, 0.14f, 1.0f }).Succeeded()
				|| !AGP::PresentRendererHostFrame().Succeeded())
			{
				result = Fail("Renderer targets did not recover after the injected resize failure.");
			}
		}
	}
	else
	{
		result = Fail("Unknown fault-test mode.");
	}

	DestroyWindow(window);
	UnregisterClassW(className, instance);
	return result;
}
