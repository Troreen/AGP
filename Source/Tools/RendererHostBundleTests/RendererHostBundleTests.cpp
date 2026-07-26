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
		|| std::string_view(invalidInitialization.Code) != "renderer.invalid_native_window")
	{
		return fail("Renderer host accepted a null native window.");
	}
	AGP::RendererResourceHandle notInitializedHandle = AGP::InvalidRendererResourceHandle;
	if (AGP::GetRendererHostNativeD3D11View().Device != nullptr
		|| AGP::ResizeRendererHost(640, 360).Status != AGP::RendererHostStatus::NotInitialized
		|| AGP::CreateRendererStaticMesh({}, notInitializedHandle).Status != AGP::RendererHostStatus::NotInitialized
		|| AGP::BeginRendererHostFrame(std::array{ 0.0f, 0.0f, 0.0f, 1.0f }).Status != AGP::RendererHostStatus::NotInitialized
		|| AGP::PresentRendererHostFrame().Status != AGP::RendererHostStatus::NotInitialized)
	{
		return fail("Renderer host exposed operations before successful initialization.");
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
	const std::filesystem::path missingShaderRoot = shaderRoot / "missing-root";
	const AGP::RendererHostResult missingShaders = AGP::InitializeRendererHost(window, missingShaderRoot.c_str(), environmentTexture.c_str());
	const std::filesystem::path missingEnvironment = environmentTexture.parent_path() / "missing.dds";
	const AGP::RendererHostResult missingEnvironmentResult = AGP::InitializeRendererHost(window, shaderRoot.c_str(), missingEnvironment.c_str());
	const AGP::RendererHostResult invalidEnvironment = AGP::InitializeRendererHost(window, shaderRoot.c_str(), (shaderRoot / "Internal" / "FullTexture_VS.hlsl").c_str());
	if (std::string_view(missingShaders.Code) != "renderer.shader_root_missing"
		|| std::string_view(missingShaders.Message).find("missing-root") == std::string_view::npos
		|| std::string_view(missingEnvironmentResult.Code) != "renderer.environment_missing"
		|| std::string_view(missingEnvironmentResult.Message).find("missing.dds") == std::string_view::npos
		|| std::string_view(invalidEnvironment.Code) != "renderer.environment_invalid"
		|| std::string_view(invalidEnvironment.Message).find("FullTexture_VS.hlsl") == std::string_view::npos)
	{
		result = fail("Renderer initialization diagnostics did not retain stable codes and relevant paths.");
	}
	else if (!AGP::InitializeRendererHost(window, shaderRoot.c_str(), environmentTexture.c_str()).Succeeded())
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
		AGP::RendererResourceHandle meshHandle = AGP::InvalidRendererResourceHandle;
		AGP::RendererResourceHandle materialHandle = AGP::InvalidRendererResourceHandle;
		const std::array vertices = {
			AGP::RendererStaticMeshVertex{ .Position = { -50.0f, -50.0f, 0.0f, 1.0f }, .UV0 = { 0.0f, 1.0f } },
			AGP::RendererStaticMeshVertex{ .Position = { 0.0f, 50.0f, 0.0f, 1.0f }, .UV0 = { 0.5f, 0.0f } },
			AGP::RendererStaticMeshVertex{ .Position = { 50.0f, -50.0f, 0.0f, 1.0f }, .UV0 = { 1.0f, 1.0f } }
		};
		constexpr std::array<std::uint32_t, 3> indices = { 0, 1, 2 };
		constexpr std::array submeshes = {
			AGP::RendererStaticMeshSubmesh{ .VertexCount = 3, .IndexCount = 3 }
		};
		const AGP::RendererStaticMeshDescription meshDescription = {
			.Vertices = vertices.data(),
			.VertexCount = vertices.size(),
			.Indices = indices.data(),
			.IndexCount = indices.size(),
			.Submeshes = submeshes.data(),
			.SubmeshCount = submeshes.size()
		};
		const std::filesystem::path fixtureRoot = environmentTexture.parent_path();
		const std::filesystem::path albedoTexture = fixtureRoot / "T_Chest_C.dds";
		const std::filesystem::path normalTexture = fixtureRoot / "T_Chest_N.dds";
		const std::filesystem::path materialTexture = fixtureRoot / "T_Chest_M.dds";
		const AGP::RendererLitMaterialDescription materialDescription = {
			.AlbedoTexture = albedoTexture.c_str(),
			.NormalTexture = normalTexture.c_str(),
			.MaterialTexture = materialTexture.c_str()
		};
		if (AGP::CreateRendererStaticMesh({}, meshHandle).Succeeded()
			|| meshHandle != AGP::InvalidRendererResourceHandle)
		{
			result = fail("Renderer host accepted an empty static-mesh description.");
		}
		else if (!AGP::CreateRendererStaticMesh(meshDescription, meshHandle).Succeeded()
			|| !AGP::CreateRendererLitMaterial(materialDescription, materialHandle).Succeeded())
		{
			result = fail("Renderer host could not create staged scene resources.");
		}
		else if (AGP::RenderRendererSceneSnapshot({}).Status != AGP::RendererHostStatus::SceneSubmissionFailed)
		{
			result = fail("Renderer host accepted scene submission before frame begin.");
		}
		else
		{
			const AGP::RendererSceneItem item = {
				.Mesh = meshHandle,
				.Material = materialHandle
			};
			const AGP::RendererSceneSnapshot snapshot = {
				.Camera = {
					.PositionCentimeters = { 0.0f, 0.0f, -300.0f },
					.VerticalFieldOfViewDegrees = 60.0f,
					.AspectRatio = 640.0f / 360.0f,
					.NearPlaneCentimeters = 1.0f,
					.FarPlaneCentimeters = 10000.0f
				},
				.Items = &item,
				.ItemCount = 1
			};
			if (!AGP::BeginRendererHostFrame(std::array{ 0.05f, 0.07f, 0.10f, 1.0f }).Succeeded()
				|| !AGP::RenderRendererSceneSnapshot(snapshot).Succeeded()
				|| !AGP::PresentRendererHostFrame().Succeeded())
			{
				result = fail("The renderer host could not submit and present a scene snapshot.");
			}
			const AGP::RendererSceneStats stats = AGP::GetRendererSceneStats();
			if (result == 0 && (stats.TotalRenderItems != 1 || stats.VisibleRenderItems != 1
				|| stats.ShadowCasters != 1 || stats.TotalLights != 1))
			{
				result = fail("Renderer scene statistics did not prove the submitted immutable snapshot.");
			}
		}

		if (result == 0 && (!AGP::ReleaseRendererResource(meshHandle).Succeeded()
			|| !AGP::ReleaseRendererResource(materialHandle).Succeeded()))
		{
			result = fail("Renderer host could not release staged scene resources.");
		}
		else if (result == 0 && !AGP::ResizeRendererHost(800, 450).Succeeded())
		{
			result = fail("The renderer host could not resize after scene submission.");
		}
		else if (result == 0 && (!AGP::BeginRendererHostFrame(std::array{ 0.08f, 0.10f, 0.14f, 1.0f }).Succeeded()
			|| !AGP::RenderRendererSceneSnapshot({}).Succeeded()
			|| !AGP::PresentRendererHostFrame().Succeeded()))
		{
			result = fail("The renderer host could not render an empty snapshot after resize.");
		}
	}

	DestroyWindow(window);
	UnregisterClassW(className, instance);
	return result;
}
