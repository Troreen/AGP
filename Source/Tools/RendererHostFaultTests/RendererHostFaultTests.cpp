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
	else if (mode == L"scene-resource-preparation")
	{
		if (!AGP::InitializeRendererHost(window, shaderRoot.c_str(), environmentTexture.c_str()).Succeeded())
		{
			result = Fail("Could not initialize the scene fault-test renderer.");
		}
		else
		{
			const std::array vertices = {
				AGP::RendererStaticMeshVertex{ .Position = { -50.0f, -50.0f, 0.0f, 1.0f } },
				AGP::RendererStaticMeshVertex{ .Position = { 0.0f, 50.0f, 0.0f, 1.0f } },
				AGP::RendererStaticMeshVertex{ .Position = { 50.0f, -50.0f, 0.0f, 1.0f } }
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
			AGP::RendererResourceHandle meshHandle = AGP::InvalidRendererResourceHandle;
			AGP::RendererResourceHandle materialHandle = AGP::InvalidRendererResourceHandle;
			if (!AGP::CreateRendererStaticMesh(meshDescription, meshHandle).Succeeded()
				|| !AGP::CreateRendererLitMaterial(materialDescription, materialHandle).Succeeded())
			{
				result = Fail("Could not create resources for the scene preparation fault test.");
			}
			else
			{
				const AGP::RendererSceneItem item = { .Mesh = meshHandle, .Material = materialHandle };
				const AGP::RendererSceneSnapshot snapshot = {
					.Camera = {
						.PositionCentimeters = { 0.0f, 0.0f, -300.0f },
						.RotationDegrees = { .Yaw = 7.0f, .Pitch = 13.0f, .Roll = 19.0f },
						.AspectRatio = 640.0f / 360.0f
					},
					.Items = &item,
					.ItemCount = 1
				};
				if (!AGP::BeginRendererHostFrame(std::array{ 0.05f, 0.07f, 0.10f, 1.0f }).Succeeded())
				{
					result = Fail("Could not begin the scene preparation fault-test frame.");
				}
				else
				{
					AGP::Testing::SetRendererHostFault(AGP::Testing::RendererHostFault::BeforeMeshBufferPreparation);
					const AGP::RendererHostResult submission = AGP::RenderRendererSceneSnapshot(snapshot);
					const AGP::RendererSceneStats stats = AGP::GetRendererSceneStats();
					if (submission.Status != AGP::RendererHostStatus::SceneSubmissionFailed
						|| std::string_view(submission.Code) != "renderer.scene_resource_preparation_failed"
						|| stats.TotalRenderItems != 0 || stats.VisibleRenderItems != 0
						|| stats.ShadowCasters != 0 || stats.TotalLights != 0)
					{
						result = Fail("Scene mesh preparation failure was reported as success or published scene statistics.");
					}
					else if (!AGP::PresentRendererHostFrame().Succeeded())
					{
						result = Fail("A rejected scene submission left the host frame unusable.");
					}
				}
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
