#include "GraphicsEngine.pch.h"

#include "RendererHost.h"
#include "GameFramework/LightComponent.h"
#include "Materials/Material.h"
#include "Objects/Mesh.h"
#include "Objects/Vertex.h"

#if defined(AGP_RENDERER_HOST_TEST_FAULTS)
#include "RendererHostFaultInjection.h"
#endif

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
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
		std::filesystem::path ourShaderRoot;
		GraphicsCommandList ourSceneCommandList;
		bool ourSceneCommandListReady = false;
		bool ourFrameBegun = false;
		RendererResourceHandle ourNextResourceHandle = 1;
		std::unordered_map<RendererResourceHandle, std::shared_ptr<Mesh>> ourMeshes;
		std::unordered_map<RendererResourceHandle, std::shared_ptr<Material>> ourMaterials;

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

		bool IsFinite(float aValue)
		{
			return std::isfinite(aValue);
		}

		bool IsFinite(const RendererFloat3& aValue)
		{
			return IsFinite(aValue.X) && IsFinite(aValue.Y) && IsFinite(aValue.Z);
		}

		CU::Vector3f ToVector3(const RendererFloat3& aValue)
		{
			return { aValue.X, aValue.Y, aValue.Z };
		}

		RendererResourceHandle AllocateResourceHandle()
		{
			while (ourNextResourceHandle == InvalidRendererResourceHandle
				|| ourMeshes.contains(ourNextResourceHandle)
				|| ourMaterials.contains(ourNextResourceHandle))
			{
				++ourNextResourceHandle;
			}
			return ourNextResourceHandle++;
		}

		bool IsValidTextureInput(const wchar_t* aPath)
		{
			return aPath != nullptr && *aPath != L'\0'
				&& std::filesystem::is_regular_file(aPath)
				&& IsDdsFile(aPath);
		}

		float MaxAxisScale(const CU::Matrix4f& aTransform)
		{
			const CU::Vector3f axisX(aTransform(1, 1), aTransform(1, 2), aTransform(1, 3));
			const CU::Vector3f axisY(aTransform(2, 1), aTransform(2, 2), aTransform(2, 3));
			const CU::Vector3f axisZ(aTransform(3, 1), aTransform(3, 2), aTransform(3, 3));
			return (std::max)({ axisX.Length(), axisY.Length(), axisZ.Length() });
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
				if (!GraphicsEngine::Get().CreateCommandList("Renderer Host Scene", ourSceneCommandList))
				{
					return Failed(RendererHostStatus::InitializationFailed, "renderer.scene_command_list_failed",
						"AGP created the renderer but could not create its scene command list; restart the process.");
				}
				ourShaderRoot = shaderRoot;
				ourSceneCommandListReady = true;
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
		ourFrameBegun = false;
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

	RendererHostResult CreateRendererStaticMesh(
		const RendererStaticMeshDescription& aDescription,
		RendererResourceHandle& outHandle) noexcept
	{
		outHandle = InvalidRendererResourceHandle;
		if (ourHostState != HostState::Ready)
		{
			return NotReady("create a static-mesh resource");
		}
		if (aDescription.Vertices == nullptr || aDescription.VertexCount == 0
			|| aDescription.Indices == nullptr || aDescription.IndexCount == 0
			|| aDescription.Submeshes == nullptr || aDescription.SubmeshCount == 0)
		{
			return Failed(RendererHostStatus::InvalidArgument, "renderer.mesh_data_required",
				"Static-mesh creation requires non-empty vertex, index, and submesh arrays.");
		}
		if (aDescription.VertexCount > (std::numeric_limits<unsigned>::max)()
			|| aDescription.IndexCount > (std::numeric_limits<unsigned>::max)())
		{
			return Failed(RendererHostStatus::InvalidArgument, "renderer.mesh_data_too_large",
				"Static-mesh vertex and index counts must fit 32-bit renderer ranges.");
		}

		try
		{
			std::vector<Vertex> vertices;
			vertices.reserve(aDescription.VertexCount);
			for (std::size_t index = 0; index < aDescription.VertexCount; ++index)
			{
				const RendererStaticMeshVertex& source = aDescription.Vertices[index];
				if (!IsFinite({ source.Position.X, source.Position.Y, source.Position.Z })
					|| !IsFinite({ source.Normal.X, source.Normal.Y, source.Normal.Z })
					|| !IsFinite({ source.Tangent.X, source.Tangent.Y, source.Tangent.Z }))
				{
					return Failed(RendererHostStatus::InvalidArgument, "renderer.mesh_non_finite",
						"Static-mesh positions, normals, and tangents must contain finite values.");
				}
				Vertex vertex;
				vertex.Position = { source.Position.X, source.Position.Y, source.Position.Z, source.Position.W };
				vertex.Color = { source.Color.X, source.Color.Y, source.Color.Z, source.Color.W };
				vertex.UV0 = { source.UV0.X, source.UV0.Y };
				vertex.UV1 = { source.UV1.X, source.UV1.Y };
				vertex.Normal = { source.Normal.X, source.Normal.Y, source.Normal.Z };
				vertex.Tangent = { source.Tangent.X, source.Tangent.Y, source.Tangent.Z };
				vertices.emplace_back(vertex);
			}

			std::vector<unsigned> indices;
			indices.reserve(aDescription.IndexCount);
			for (std::size_t index = 0; index < aDescription.IndexCount; ++index)
			{
				if (aDescription.Indices[index] >= aDescription.VertexCount)
				{
					return Failed(RendererHostStatus::InvalidArgument, "renderer.mesh_index_out_of_range",
						"Static-mesh indices must reference an available vertex.");
				}
				indices.emplace_back(aDescription.Indices[index]);
			}

			std::vector<Mesh::Element> elements;
			elements.reserve(aDescription.SubmeshCount);
			for (std::size_t index = 0; index < aDescription.SubmeshCount; ++index)
			{
				const RendererStaticMeshSubmesh& source = aDescription.Submeshes[index];
				const std::uint64_t vertexEnd = static_cast<std::uint64_t>(source.VertexOffset) + source.VertexCount;
				const std::uint64_t indexEnd = static_cast<std::uint64_t>(source.IndexOffset) + source.IndexCount;
				if (source.VertexCount == 0 || source.IndexCount == 0
					|| vertexEnd > aDescription.VertexCount || indexEnd > aDescription.IndexCount)
				{
					return Failed(RendererHostStatus::InvalidArgument, "renderer.mesh_submesh_out_of_range",
						"Every static-mesh submesh must describe non-empty ranges inside the supplied arrays.");
				}
				elements.push_back({
					.VertexOffset = source.VertexOffset,
					.IndexOffset = source.IndexOffset,
					.NumVertices = source.VertexCount,
					.NumIndices = source.IndexCount,
					.MaterialIndex = 0
				});
			}

			const RendererResourceHandle handle = AllocateResourceHandle();
			auto mesh = std::make_shared<Mesh>();
			mesh->Initialize("renderer-host-mesh", std::move(elements), std::move(vertices), std::move(indices));
			ourMeshes.emplace(handle, std::move(mesh));
			outHandle = handle;
			return Completed();
		}
		catch (const std::exception& exception)
		{
			return Failed(RendererHostStatus::ResourceCreationFailed, "renderer.mesh_creation_exception",
				std::string("AGP threw while creating a static-mesh resource: ") + exception.what());
		}
		catch (...)
		{
			return Failed(RendererHostStatus::ResourceCreationFailed, "renderer.mesh_creation_exception",
				"AGP threw an unknown exception while creating a static-mesh resource.");
		}
	}

	RendererHostResult CreateRendererLitMaterial(
		const RendererLitMaterialDescription& aDescription,
		RendererResourceHandle& outHandle) noexcept
	{
		outHandle = InvalidRendererResourceHandle;
		if (ourHostState != HostState::Ready)
		{
			return NotReady("create a material resource");
		}
		if (aDescription.Preset != SurfaceLitOpaquePreset)
		{
			return Failed(RendererHostStatus::InvalidArgument, "renderer.material_preset_unsupported",
				"The renderer host supports only the surface_lit_opaque material preset in this contract.");
		}
		try
		{
			if (!IsValidTextureInput(aDescription.AlbedoTexture)
				|| !IsValidTextureInput(aDescription.NormalTexture)
				|| !IsValidTextureInput(aDescription.MaterialTexture))
			{
				return Failed(RendererHostStatus::InvalidArgument, "renderer.material_texture_invalid",
					"The lit material requires accessible DDS albedo, normal, and packed material textures.");
			}

			MaterialDescription description;
			description.Name = "renderer-host-surface-lit-opaque";
			description.Domain = MaterialDomain::Surface;
			description.ShadingModel = ShadingModel::Lit;
			description.BlendMode = BlendMode::Opaque;
			description.MaterialShaderCode = ourShaderRoot / "Material" / "Material.hlsli";
			description.AlbedoTexture = aDescription.AlbedoTexture;
			description.NormalTexture = aDescription.NormalTexture;
			description.MaterialTexture = aDescription.MaterialTexture;

			auto material = std::make_shared<Material>();
			if (!GraphicsEngine::Get().CreateMaterial(description, *material))
			{
				return Failed(RendererHostStatus::ResourceCreationFailed, "renderer.material_creation_failed",
					"AGP could not compile the surface_lit_opaque preset or load its DDS textures.");
			}
			const RendererResourceHandle handle = AllocateResourceHandle();
			ourMaterials.emplace(handle, std::move(material));
			outHandle = handle;
			return Completed();
		}
		catch (const std::exception& exception)
		{
			return Failed(RendererHostStatus::ResourceCreationFailed, "renderer.material_creation_exception",
				std::string("AGP threw while creating a material resource: ") + exception.what());
		}
		catch (...)
		{
			return Failed(RendererHostStatus::ResourceCreationFailed, "renderer.material_creation_exception",
				"AGP threw an unknown exception while creating a material resource.");
		}
	}

	RendererHostResult ReleaseRendererResource(RendererResourceHandle aHandle) noexcept
	{
		if (ourHostState != HostState::Ready)
		{
			return NotReady("release a renderer resource");
		}
		if (aHandle == InvalidRendererResourceHandle)
		{
			return Failed(RendererHostStatus::InvalidArgument, "renderer.resource_handle_invalid",
				"A non-zero renderer resource handle is required.");
		}
		if (ourMeshes.erase(aHandle) == 0 && ourMaterials.erase(aHandle) == 0)
		{
			return Failed(RendererHostStatus::InvalidResource, "renderer.resource_not_found",
				"The renderer resource handle is not live.");
		}
		return Completed();
	}

	RendererHostResult BeginRendererHostFrame(const std::array<float, 4>& aClearColor) noexcept
	{
		ourFrameBegun = false;
		if (ourHostState != HostState::Ready)
		{
			return NotReady("begin a frame");
		}
		try
		{
			if (GraphicsEngine::Get().BeginBackBufferFrame(aClearColor))
			{
				ourFrameBegun = true;
				return Completed();
			}
			return Failed(RendererHostStatus::NotInitialized, "renderer.not_initialized", "AGP cannot begin a frame before renderer initialization succeeds.");
		}
		catch (...)
		{
			return Failed(RendererHostStatus::BeginFrameFailed, "renderer.begin_frame_failed", "AGP threw while binding and clearing its backbuffer.");
		}
	}

	RendererHostResult RenderRendererSceneSnapshot(const RendererSceneSnapshot& aSnapshot) noexcept
	{
		if (ourHostState != HostState::Ready)
		{
			return NotReady("render a scene snapshot");
		}
		if (!ourFrameBegun || !ourSceneCommandListReady)
		{
			return Failed(RendererHostStatus::SceneSubmissionFailed, "renderer.frame_not_begun",
				"BeginRendererHostFrame must succeed before scene snapshot submission.");
		}
		if ((aSnapshot.Items == nullptr) != (aSnapshot.ItemCount == 0))
		{
			return Failed(RendererHostStatus::InvalidArgument, "renderer.scene_items_invalid",
				"Scene items must provide either a non-null array with a positive count or an empty null range.");
		}
		if (aSnapshot.ItemCount > (std::numeric_limits<std::uint32_t>::max)())
		{
			return Failed(RendererHostStatus::InvalidArgument, "renderer.scene_too_large",
				"Scene snapshot item count must fit the renderer's 32-bit statistics contract.");
		}
		const RendererPerspectiveCamera& camera = aSnapshot.Camera;
		if (!IsFinite(camera.PositionCentimeters) || !IsFinite(camera.RotationDegrees)
			|| !IsFinite(camera.VerticalFieldOfViewDegrees) || camera.VerticalFieldOfViewDegrees <= 1.0f
			|| camera.VerticalFieldOfViewDegrees >= 179.0f || !IsFinite(camera.AspectRatio) || camera.AspectRatio <= 0.0f
			|| !IsFinite(camera.NearPlaneCentimeters) || !IsFinite(camera.FarPlaneCentimeters)
			|| camera.NearPlaneCentimeters <= 0.0f || camera.FarPlaneCentimeters <= camera.NearPlaneCentimeters)
		{
			return Failed(RendererHostStatus::InvalidArgument, "renderer.scene_camera_invalid",
				"The perspective camera requires finite position/rotation, FOV in (1,179), positive aspect/near plane, and far greater than near.");
		}

		try
		{
			GraphicsEngine::RenderSceneSnapshot internal;
			internal.HasCamera = true;
			internal.Camera.SetPerspective(
				camera.VerticalFieldOfViewDegrees,
				camera.AspectRatio,
				camera.NearPlaneCentimeters,
				camera.FarPlaneCentimeters);
			internal.Camera.GetTransform().SetPosition(ToVector3(camera.PositionCentimeters));
			internal.Camera.GetTransform().SetRotation(ToVector3(camera.RotationDegrees));
			internal.ShadowCasters.reserve(aSnapshot.ItemCount);
			internal.VisibleRenderItems.reserve(aSnapshot.ItemCount);

			for (std::size_t index = 0; index < aSnapshot.ItemCount; ++index)
			{
				const RendererSceneItem& source = aSnapshot.Items[index];
				const auto mesh = ourMeshes.find(source.Mesh);
				const auto material = ourMaterials.find(source.Material);
				if (mesh == ourMeshes.end() || material == ourMaterials.end())
				{
					return Failed(RendererHostStatus::InvalidResource, "renderer.scene_resource_not_found",
						"Every scene item must reference live mesh and material resource handles.");
				}
				if (!IsFinite(source.Transform.PositionCentimeters)
					|| !IsFinite(source.Transform.RotationDegrees) || !IsFinite(source.Transform.Scale)
					|| source.Transform.Scale.X <= 0.0f || source.Transform.Scale.Y <= 0.0f || source.Transform.Scale.Z <= 0.0f)
				{
					return Failed(RendererHostStatus::InvalidArgument, "renderer.scene_transform_invalid",
						"Scene item transforms require finite values and strictly positive scale.");
				}

				CU::Transform transform;
				transform.SetPosition(ToVector3(source.Transform.PositionCentimeters));
				transform.SetRotation(ToVector3(source.Transform.RotationDegrees));
				transform.SetScale(ToVector3(source.Transform.Scale));

				GraphicsEngine::RenderItemSnapshot item;
				item.Mesh = mesh->second;
				item.Materials.assign(mesh->second->GetNumMaterialSlots(), material->second);
				item.World = transform.GetWorldMatrix();
				item.HasBounds = mesh->second->HasLocalBounds();
				if (item.HasBounds)
				{
					item.BoundsCenter = CU::Maths::TransformPoint(mesh->second->GetLocalBoundsCenter(), item.World);
					item.BoundsRadius = mesh->second->GetLocalBoundsRadius() * MaxAxisScale(item.World);
					item.HasBounds = IsFinite(item.BoundsRadius);
				}
				internal.VisibleRenderItems.emplace_back(item);
				if (source.CastsShadows)
				{
					internal.ShadowCasters.emplace_back(std::move(item));
				}
			}

			const RendererDirectionalLight& sourceLight = aSnapshot.DirectionalLight;
			if (!IsFinite(sourceLight.Color) || !IsFinite(sourceLight.Direction)
				|| !IsFinite(sourceLight.Intensity) || sourceLight.Intensity < 0.0f
				|| ToVector3(sourceLight.Direction).LengthSqr() == 0.0f)
			{
				return Failed(RendererHostStatus::InvalidArgument, "renderer.scene_light_invalid",
					"The directional light requires finite color, non-negative intensity, and a non-zero finite direction.");
			}
			GraphicsEngine::LightSnapshot light;
			light.Type = LightType::Directional;
			light.Color = ToVector3(sourceLight.Color);
			light.Intensity = sourceLight.Intensity;
			light.Direction = ToVector3(sourceLight.Direction).GetNormalized();
			internal.RelevantLights.emplace_back(light);
			internal.Stats.TotalRenderItems = static_cast<std::uint32_t>(internal.VisibleRenderItems.size());
			internal.Stats.VisibleRenderItems = static_cast<std::uint32_t>(internal.VisibleRenderItems.size());
			internal.Stats.ShadowCasters = static_cast<std::uint32_t>(internal.ShadowCasters.size());
			internal.Stats.TotalLights = 1;
			internal.Stats.RelevantLights = 1;

			ourSceneCommandList.ResetCommandList();
			GraphicsEngine::Get().RenderSnapshot(ourSceneCommandList, internal);
			ourSceneCommandList.FinishCommandList();
			if (!ourSceneCommandList.IsReadyForExecution())
			{
				return Failed(RendererHostStatus::SceneSubmissionFailed, "renderer.scene_command_recording_failed",
					"AGP could not finish the scene snapshot command list.");
			}
			GraphicsEngine::Get().ExecuteCommandList(ourSceneCommandList);
			ourSceneCommandList.ResetCommandList();
			return Completed();
		}
		catch (const std::exception& exception)
		{
			ourSceneCommandList.ResetCommandList();
			return Failed(RendererHostStatus::SceneSubmissionFailed, "renderer.scene_submission_exception",
				std::string("AGP threw while submitting a scene snapshot: ") + exception.what());
		}
		catch (...)
		{
			ourSceneCommandList.ResetCommandList();
			return Failed(RendererHostStatus::SceneSubmissionFailed, "renderer.scene_submission_exception",
				"AGP threw an unknown exception while submitting a scene snapshot.");
		}
	}

	RendererSceneStats GetRendererSceneStats() noexcept
	{
		if (ourHostState != HostState::Ready)
		{
			return {};
		}
		const GraphicsEngine::RenderStats stats = GraphicsEngine::Get().GetLastRenderStats();
		return {
			.TotalRenderItems = stats.TotalRenderItems,
			.VisibleRenderItems = stats.VisibleRenderItems,
			.ShadowCasters = stats.ShadowCasters,
			.TotalLights = stats.TotalLights
		};
	}

	RendererHostResult PresentRendererHostFrame() noexcept
	{
		if (ourHostState != HostState::Ready)
		{
			return NotReady("present");
		}
		try
		{
			ourFrameBegun = false;
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
