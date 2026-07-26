#pragma once

#include <array>
#include <cstdint>
#include <string_view>

struct ID3D11Device;
struct ID3D11DeviceContext;

namespace AGP
{
	inline constexpr std::string_view RendererHostVersion = "agp-renderer-host/1.1.0";

	struct NativeD3D11View
	{
		ID3D11Device* Device = nullptr;
		ID3D11DeviceContext* ImmediateContext = nullptr;
	};

	enum class RendererHostStatus : std::uint8_t
	{
		Completed,
		InvalidArgument,
		NotInitialized,
		InitializationFailed,
		ResizeFailed,
		RendererUnavailable,
		BeginFrameFailed,
		PresentFailed
	};

	struct RendererHostResult
	{
		RendererHostStatus Status = RendererHostStatus::Completed;
		char Code[64] = "renderer.completed";
		char Message[768] = "The renderer-host operation completed.";

		[[nodiscard]] bool Succeeded() const { return Status == RendererHostStatus::Completed; }
	};

	// The host owns the native window and message loop. AGP owns D3D11 and
	// presentation; these raw handles remain borrowed for AGP's lifetime.
	[[nodiscard]] RendererHostResult InitializeRendererHost(
		void* aNativeWindow,
		const wchar_t* aShaderRoot,
		const wchar_t* aEnvironmentTexture) noexcept;
	[[nodiscard]] NativeD3D11View GetRendererHostNativeD3D11View() noexcept;
	[[nodiscard]] RendererHostResult ResizeRendererHost(unsigned aWidth, unsigned aHeight) noexcept;
	[[nodiscard]] RendererHostResult BeginRendererHostFrame(const std::array<float, 4>& aClearColor) noexcept;
	[[nodiscard]] RendererHostResult PresentRendererHostFrame() noexcept;
}
