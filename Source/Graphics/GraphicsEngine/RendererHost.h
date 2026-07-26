#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

struct ID3D11Device;
struct ID3D11DeviceContext;

namespace AGP
{
	inline constexpr std::string_view RendererHostVersion = "agp-renderer-host/1.2.0";
	inline constexpr std::string_view SurfaceLitOpaquePreset = "surface_lit_opaque";

	using RendererResourceHandle = std::uint64_t;
	inline constexpr RendererResourceHandle InvalidRendererResourceHandle = 0;

	struct RendererFloat2
	{
		float X = 0.0f;
		float Y = 0.0f;
	};

	struct RendererFloat3
	{
		float X = 0.0f;
		float Y = 0.0f;
		float Z = 0.0f;
	};

	struct RendererFloat4
	{
		float X = 0.0f;
		float Y = 0.0f;
		float Z = 0.0f;
		float W = 0.0f;
	};

	struct RendererStaticMeshVertex
	{
		RendererFloat4 Position = { 0.0f, 0.0f, 0.0f, 1.0f };
		RendererFloat4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
		RendererFloat2 UV0;
		RendererFloat2 UV1;
		RendererFloat3 Normal = { 0.0f, 0.0f, 1.0f };
		RendererFloat3 Tangent = { 1.0f, 0.0f, 0.0f };
	};

	struct RendererStaticMeshSubmesh
	{
		std::uint32_t VertexOffset = 0;
		std::uint32_t IndexOffset = 0;
		std::uint32_t VertexCount = 0;
		std::uint32_t IndexCount = 0;
	};

	struct RendererStaticMeshDescription
	{
		const RendererStaticMeshVertex* Vertices = nullptr;
		std::size_t VertexCount = 0;
		const std::uint32_t* Indices = nullptr;
		std::size_t IndexCount = 0;
		const RendererStaticMeshSubmesh* Submeshes = nullptr;
		std::size_t SubmeshCount = 0;
	};

	struct RendererLitMaterialDescription
	{
		std::string_view Preset = SurfaceLitOpaquePreset;
		const wchar_t* AlbedoTexture = nullptr;
		const wchar_t* NormalTexture = nullptr;
		const wchar_t* MaterialTexture = nullptr;
	};

	struct RendererTransform
	{
		RendererFloat3 PositionCentimeters;
		RendererFloat3 RotationDegrees;
		RendererFloat3 Scale = { 1.0f, 1.0f, 1.0f };
	};

	struct RendererSceneItem
	{
		RendererResourceHandle Mesh = InvalidRendererResourceHandle;
		RendererResourceHandle Material = InvalidRendererResourceHandle;
		RendererTransform Transform;
		bool CastsShadows = true;
	};

	struct RendererPerspectiveCamera
	{
		RendererFloat3 PositionCentimeters = { 0.0f, 150.0f, -300.0f };
		RendererFloat3 RotationDegrees = { 0.0f, 0.0f, 0.0f };
		float VerticalFieldOfViewDegrees = 60.0f;
		float AspectRatio = 16.0f / 9.0f;
		float NearPlaneCentimeters = 1.0f;
		float FarPlaneCentimeters = 100000.0f;
	};

	struct RendererDirectionalLight
	{
		RendererFloat3 Color = { 1.0f, 1.0f, 1.0f };
		float Intensity = 1.0f;
		RendererFloat3 Direction = { 0.4f, -0.8f, 0.3f };
	};

	struct RendererSceneSnapshot
	{
		RendererPerspectiveCamera Camera;
		const RendererSceneItem* Items = nullptr;
		std::size_t ItemCount = 0;
		RendererDirectionalLight DirectionalLight;
	};

	struct RendererSceneStats
	{
		std::uint32_t TotalRenderItems = 0;
		std::uint32_t VisibleRenderItems = 0;
		std::uint32_t ShadowCasters = 0;
		std::uint32_t TotalLights = 0;
	};

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
		PresentFailed,
		ResourceCreationFailed,
		InvalidResource,
		SceneSubmissionFailed
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
	[[nodiscard]] RendererHostResult CreateRendererStaticMesh(
		const RendererStaticMeshDescription& aDescription,
		RendererResourceHandle& outHandle) noexcept;
	[[nodiscard]] RendererHostResult CreateRendererLitMaterial(
		const RendererLitMaterialDescription& aDescription,
		RendererResourceHandle& outHandle) noexcept;
	[[nodiscard]] RendererHostResult ReleaseRendererResource(RendererResourceHandle aHandle) noexcept;
	[[nodiscard]] RendererHostResult BeginRendererHostFrame(const std::array<float, 4>& aClearColor) noexcept;
	[[nodiscard]] RendererHostResult RenderRendererSceneSnapshot(
		const RendererSceneSnapshot& aSnapshot) noexcept;
	[[nodiscard]] RendererSceneStats GetRendererSceneStats() noexcept;
	[[nodiscard]] RendererHostResult PresentRendererHostFrame() noexcept;
}
