#include "GraphicsEngine.pch.h"

#include "RendererHost.h"

#include <filesystem>
#include <Windows.h>

namespace AGP
{
	namespace
	{
		RendererHostResult Completed()
		{
			return {};
		}

		RendererHostResult Failed(RendererHostStatus aStatus, const char* aCode, const char* aMessage)
		{
			return { aStatus, aCode, aMessage };
		}
	}

	RendererHostResult InitializeRendererHost(
		void* aNativeWindow,
		const wchar_t* aShaderRoot,
		const wchar_t* aEnvironmentTexture) noexcept
	{
		if (aNativeWindow == nullptr
			|| aShaderRoot == nullptr || *aShaderRoot == L'\0'
			|| aEnvironmentTexture == nullptr || *aEnvironmentTexture == L'\0')
		{
			return Failed(RendererHostStatus::InvalidArgument, "renderer.invalid_argument", "A native window, shader root, and environment texture are required.");
		}

		try
		{
			if (GraphicsEngine::Get().Initialize(
				static_cast<HWND>(aNativeWindow),
				std::filesystem::path(aShaderRoot),
				std::filesystem::path(aEnvironmentTexture)))
			{
				return Completed();
			}
			return Failed(RendererHostStatus::InitializationFailed, "renderer.initialization_failed", "AGP could not initialize its D3D11 renderer resources.");
		}
		catch (...)
		{
			return Failed(RendererHostStatus::InitializationFailed, "renderer.initialization_failed", "AGP threw while initializing its D3D11 renderer resources.");
		}
	}

	NativeD3D11View GetRendererHostNativeD3D11View() noexcept
	{
		GraphicsEngine& graphicsEngine = GraphicsEngine::Get();
		return {
			.Device = graphicsEngine.GetNativeDevice(),
			.ImmediateContext = graphicsEngine.GetNativeImmediateContext()
		};
	}

	RendererHostResult ResizeRendererHost(unsigned aWidth, unsigned aHeight) noexcept
	{
		if (aWidth == 0 || aHeight == 0)
		{
			return Failed(RendererHostStatus::InvalidArgument, "renderer.invalid_size", "Renderer dimensions must both be greater than zero.");
		}
		try
		{
			if (GraphicsEngine::Get().ResizeBackBuffer(aWidth, aHeight))
			{
				return Completed();
			}
			return Failed(RendererHostStatus::ResizeFailed, "renderer.resize_failed", "AGP could not resize its swapchain and depth resources.");
		}
		catch (...)
		{
			return Failed(RendererHostStatus::ResizeFailed, "renderer.resize_failed", "AGP threw while resizing its swapchain and depth resources.");
		}
	}

	RendererHostResult BeginRendererHostFrame(const std::array<float, 4>& aClearColor) noexcept
	{
		try
		{
			if (GraphicsEngine::Get().BeginBackBufferFrame(aClearColor))
			{
				return Completed();
			}
			return Failed(RendererHostStatus::NotInitialized, "renderer.not_initialized", "AGP cannot begin a frame before renderer initialization succeeds.");
		}
		catch (...)
		{
			return Failed(RendererHostStatus::BeginFrameFailed, "renderer.begin_frame_failed", "AGP threw while binding and clearing its backbuffer.");
		}
	}

	RendererHostResult PresentRendererHostFrame() noexcept
	{
		try
		{
			if (GetRendererHostNativeD3D11View().Device == nullptr)
			{
				return Failed(RendererHostStatus::NotInitialized, "renderer.not_initialized", "AGP cannot present before renderer initialization succeeds.");
			}
			if (GraphicsEngine::Get().Present())
			{
				return Completed();
			}
			return Failed(RendererHostStatus::PresentFailed, "renderer.present_failed", "AGP's swapchain present operation failed.");
		}
		catch (...)
		{
			return Failed(RendererHostStatus::PresentFailed, "renderer.present_failed", "AGP threw while presenting its swapchain.");
		}
	}
}
