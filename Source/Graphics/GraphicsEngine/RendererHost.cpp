#include "GraphicsEngine.pch.h"

#include "RendererHost.h"

#if defined(AGP_RENDERER_HOST_TEST_FAULTS)
#include "RendererHostFaultInjection.h"
#endif

#include <filesystem>
#include <fstream>
#include <string>
#include <Windows.h>

namespace AGP
{
#if defined(AGP_RENDERER_HOST_TEST_FAULTS)
	namespace Testing
	{
		namespace
		{
			RendererHostFault ourRendererHostFault = RendererHostFault::None;
		}

		void SetRendererHostFault(RendererHostFault aFault) noexcept
		{
			ourRendererHostFault = aFault;
		}

		bool ConsumeRendererHostFault(RendererHostFault aFault) noexcept
		{
			if (ourRendererHostFault != aFault)
			{
				return false;
			}
			ourRendererHostFault = RendererHostFault::None;
			return true;
		}
	}
#endif

	namespace
	{
		enum class HostState
		{
			Uninitialized,
			Ready,
			ResizeRecoveryRequired,
			RestartRequired
		};

		HostState ourHostState = HostState::Uninitialized;

		RendererHostResult Completed()
		{
			return {};
		}

		RendererHostResult Failed(RendererHostStatus aStatus, const char* aCode, const std::string& aMessage)
		{
			RendererHostResult result;
			result.Status = aStatus;
			strncpy_s(result.Code, aCode, _TRUNCATE);
			strncpy_s(result.Message, aMessage.c_str(), _TRUNCATE);
			return result;
		}

		std::string Utf8Path(const std::filesystem::path& aPath)
		{
			const std::u8string utf8 = aPath.u8string();
			return { reinterpret_cast<const char*>(utf8.data()), utf8.size() };
		}

		bool IsDdsFile(const std::filesystem::path& aPath)
		{
			std::ifstream stream(aPath, std::ios::binary);
			char magic[4] = {};
			return stream.read(magic, sizeof(magic)) && std::string_view(magic, sizeof(magic)) == "DDS ";
		}

		RendererHostResult NotReady(const char* aOperation)
		{
			if (ourHostState == HostState::ResizeRecoveryRequired)
			{
				return Failed(RendererHostStatus::RendererUnavailable, "renderer.resize_recovery_required",
					std::string("AGP cannot ") + aOperation + " because a resize invalidated the frame targets; call ResizeRendererHost again to recover them.");
			}
			if (ourHostState == HostState::RestartRequired)
			{
				return Failed(RendererHostStatus::RendererUnavailable, "renderer.restart_required",
					std::string("AGP cannot ") + aOperation + " after partial initialization or device failure; restart the process before retrying initialization.");
			}
			return Failed(RendererHostStatus::NotInitialized, "renderer.not_initialized",
				std::string("AGP cannot ") + aOperation + " before renderer initialization succeeds.");
		}
	}

	RendererHostResult InitializeRendererHost(
		void* aNativeWindow,
		const wchar_t* aShaderRoot,
		const wchar_t* aEnvironmentTexture) noexcept
	{
		if (ourHostState == HostState::Ready || ourHostState == HostState::ResizeRecoveryRequired)
		{
			return Failed(RendererHostStatus::InitializationFailed, "renderer.already_initialized",
				"AGP renderer initialization has already completed in this process.");
		}
		if (ourHostState == HostState::RestartRequired)
		{
			return NotReady("retry initialization");
		}
		if (aNativeWindow == nullptr || !IsWindow(static_cast<HWND>(aNativeWindow)))
		{
			return Failed(RendererHostStatus::InvalidArgument, "renderer.invalid_native_window", "A valid host-owned HWND is required.");
		}
		if (aShaderRoot == nullptr || *aShaderRoot == L'\0')
		{
			return Failed(RendererHostStatus::InvalidArgument, "renderer.shader_root_required", "A non-empty shader-root path is required.");
		}
		if (aEnvironmentTexture == nullptr || *aEnvironmentTexture == L'\0')
		{
			return Failed(RendererHostStatus::InvalidArgument, "renderer.environment_required", "A non-empty environment-texture path is required.");
		}

		try
		{
			const std::filesystem::path shaderRoot(aShaderRoot);
			const std::filesystem::path environmentTexture(aEnvironmentTexture);
			if (!std::filesystem::is_directory(shaderRoot))
			{
				return Failed(RendererHostStatus::InitializationFailed, "renderer.shader_root_missing",
					"Shader root does not name an accessible directory: " + Utf8Path(shaderRoot));
			}
			constexpr std::array<std::wstring_view, 6> requiredShaders = {
				L"Material/Surface_VS.hlsl",
				L"Material/Unlit_PS.hlsl",
				L"Material/Material.hlsli",
				L"Internal/FullTexture_VS.hlsl",
				L"Internal/BRDF_LUT_PS.hlsl",
				L"Internal/PointShadow_GS.hlsl"
			};
			for (const std::wstring_view relativeShader : requiredShaders)
			{
				const std::filesystem::path shaderPath = shaderRoot / relativeShader;
				if (!std::filesystem::is_regular_file(shaderPath))
				{
					return Failed(RendererHostStatus::InitializationFailed, "renderer.shader_missing",
						"Required renderer shader is missing: " + Utf8Path(shaderPath));
				}
			}
			if (!std::filesystem::is_regular_file(environmentTexture))
			{
				return Failed(RendererHostStatus::InitializationFailed, "renderer.environment_missing",
					"Environment texture does not name an accessible file: " + Utf8Path(environmentTexture));
			}
			if (!IsDdsFile(environmentTexture))
			{
				return Failed(RendererHostStatus::InitializationFailed, "renderer.environment_invalid",
					"Environment texture is not a valid DDS input: " + Utf8Path(environmentTexture));
			}

			// Once GraphicsEngine initialization starts, its singleton may own a partial
			// device/resource graph. A failed attempt is therefore process-terminal.
			ourHostState = HostState::RestartRequired;
			if (GraphicsEngine::Get().Initialize(
				static_cast<HWND>(aNativeWindow),
				shaderRoot,
				environmentTexture))
			{
#if defined(AGP_RENDERER_HOST_TEST_FAULTS)
				if (Testing::ConsumeRendererHostFault(Testing::RendererHostFault::AfterGraphicsInitialize))
				{
					return Failed(RendererHostStatus::InitializationFailed, "renderer.initialization_injected_failure",
						"Injected failure after AGP created its renderer resources; restart is required before retrying.");
				}
#endif
				ourHostState = HostState::Ready;
				return Completed();
			}
			return Failed(RendererHostStatus::InitializationFailed, "renderer.device_or_resource_initialization_failed",
				"AGP could not create its D3D11 device, shaders, or renderer resources. Shader root: "
				+ Utf8Path(shaderRoot) + "; environment texture: " + Utf8Path(environmentTexture));
		}
		catch (const std::exception& exception)
		{
			return Failed(RendererHostStatus::InitializationFailed, "renderer.initialization_exception",
				std::string("AGP threw while validating or initializing renderer resources: ") + exception.what());
		}
		catch (...)
		{
			return Failed(RendererHostStatus::InitializationFailed, "renderer.initialization_exception",
				"AGP threw an unknown exception while validating or initializing renderer resources.");
		}
	}

	NativeD3D11View GetRendererHostNativeD3D11View() noexcept
	{
		if (ourHostState != HostState::Ready)
		{
			return {};
		}
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
		if (ourHostState != HostState::Ready && ourHostState != HostState::ResizeRecoveryRequired)
		{
			return NotReady("resize");
		}
		try
		{
			const RenderHardwareInterface::ResizeBackBufferResult resizeResult = GraphicsEngine::Get().ResizeBackBuffer(aWidth, aHeight);
			if (resizeResult == RenderHardwareInterface::ResizeBackBufferResult::Completed)
			{
				ourHostState = HostState::Ready;
				return Completed();
			}
			if (resizeResult == RenderHardwareInterface::ResizeBackBufferResult::FailedTargetsUnavailable
				|| ourHostState == HostState::ResizeRecoveryRequired)
			{
				ourHostState = HostState::ResizeRecoveryRequired;
				return Failed(RendererHostStatus::RendererUnavailable, "renderer.resize_recovery_required",
					"AGP resized its swapchain but could not recreate valid frame targets; frame submission is blocked until ResizeRendererHost succeeds.");
			}
			return Failed(RendererHostStatus::ResizeFailed, "renderer.resize_failed_targets_preserved",
				"AGP could not resize its swapchain resources; the previous frame targets remain valid.");
		}
		catch (...)
		{
			ourHostState = HostState::RestartRequired;
			return Failed(RendererHostStatus::RendererUnavailable, "renderer.resize_exception_restart_required",
				"AGP threw while resizing swapchain resources; frame submission is blocked and process restart is required.");
		}
	}

	RendererHostResult BeginRendererHostFrame(const std::array<float, 4>& aClearColor) noexcept
	{
		if (ourHostState != HostState::Ready)
		{
			return NotReady("begin a frame");
		}
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
		if (ourHostState != HostState::Ready)
		{
			return NotReady("present");
		}
		try
		{
			if (GraphicsEngine::Get().Present())
			{
				return Completed();
			}
			ourHostState = HostState::RestartRequired;
			return Failed(RendererHostStatus::PresentFailed, "renderer.present_failed_restart_required",
				"AGP's swapchain present operation failed; device validity is unknown and process restart is required.");
		}
		catch (...)
		{
			ourHostState = HostState::RestartRequired;
			return Failed(RendererHostStatus::PresentFailed, "renderer.present_exception_restart_required",
				"AGP threw while presenting its swapchain; process restart is required.");
		}
	}
}
