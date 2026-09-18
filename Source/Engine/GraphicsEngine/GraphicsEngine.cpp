#include "GraphicsEngine.pch.h"
#include "Maths.hpp"
#include "GraphicsEngine.h"

#include "ConstantBuffers/AnimationBuffer.h"
#include "ConstantBuffers/FrameBuffer.h"
#include "ConstantBuffers/LightBuffer.h"
#include "ConstantBuffers/ObjectBuffer.h"
#include "RHI/GraphicsCommandList.h"
#include "RHI/RHIShaderReflectionInfo.h"

#include "Materials/Material.h"
#include "Materials/MaterialShaderIncludeHandler.h"
#include "Materials/MaterialHelpers.h"
#include "Objects/Shader.h"

#include <d3dcompiler.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <future>
#include <chrono>
#include "StartupOptions.h"
#include "RenderCulling.h"
#include "RenderItemRouting.h"
#include <limits>
#include <vector>

namespace
{
	using namespace RenderCulling;
	using Clock = std::chrono::steady_clock;

	double ElapsedMilliseconds(Clock::time_point start)
	{
		return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
	}

	namespace ShadowConfig
	{
		constexpr unsigned MapResolution = 2048;
		constexpr unsigned DirectionalCascadeCount = 4;
		constexpr unsigned MaxSpotMaps = 4;
		constexpr unsigned MaxPointMaps = 4;
		constexpr unsigned HighTextureSlotStart = 100;
		constexpr float BiasMin = 0.0f;
		constexpr float BiasMax = 0.005f;
		constexpr float DirectionalShaderBias = 0.00025f;
		constexpr float SpotShaderBias = 0.00008f;
		constexpr float PointShaderBias = 0.0002f;
		constexpr int DirectionalRasterDepthBias = 80;
		constexpr float DirectionalRasterSlopeBias = 0.35f;
		constexpr int LocalRasterDepthBias = 1;
		constexpr float LocalRasterSlopeBias = 0.02f;
		constexpr std::array<float, DirectionalCascadeCount> CascadeSplits = {150.0f, 600.0f, 1600.0f, 5000.0f};
		constexpr float DirectionalCascadeSplitPaddingMin = 30.0f;
		constexpr float DirectionalCascadeSplitPaddingScale = 0.08f;
		constexpr float DirectionalCascadeLightPaddingMin = 250.0f;
		constexpr float DirectionalCascadeLightPaddingScale = 0.08f;
		constexpr float DirectionalCascadeDepthPaddingScale = 0.15f;
		constexpr float DirectionalFilterRadiusWorld = 2.0f;
	}

	namespace PBLConfig
	{
		constexpr unsigned EnvironmentCubeSlot = 98;
		constexpr unsigned BRDFLUTSlot = 99;
		constexpr unsigned BRDFLUTResolution = 512;
	}

	struct PointShadowBufferData
	{
		std::array<CU::Matrix4f, 6> ViewProjection = {};
	};

	struct CascadeShadowData
	{
		CU::Matrix4f View;
		CU::Matrix4f ViewProjection;
		float MinX = 0.0f;
		float MaxX = 0.0f;
		float MinY = 0.0f;
		float MaxY = 0.0f;
		float MinZ = 0.0f;
		float MaxZ = 0.0f;
		float DepthRange = 1.0f;
		bool HasBounds = false;
	};

	std::array<CU::Vector3f, 8> GetFrustumCorners(const CU::Camera3D& aCamera, float aNearPlane, float aFarPlane);

	using RenderItemPtrList = std::vector<const GraphicsEngine::RenderItemSnapshot*>;

	bool IntersectsLightSpaceBounds(const CascadeShadowData& aCascade, const GraphicsEngine::RenderItemSnapshot& aRenderItem)
	{
		if (!aCascade.HasBounds || !aRenderItem.HasBounds)
		{
			return true;
		}

		const CU::Vector3f center = CU::Maths::TransformPoint(aRenderItem.BoundsCenter, aCascade.View);
		const float radius = aRenderItem.BoundsRadius;
		if (!IsFinite(center) || !std::isfinite(radius) || radius < 0.0f || !std::isfinite(aCascade.MinX) ||
		    !std::isfinite(aCascade.MaxX) || !std::isfinite(aCascade.MinY) || !std::isfinite(aCascade.MaxY) ||
		    !std::isfinite(aCascade.MinZ) || !std::isfinite(aCascade.MaxZ))
		{
			return true;
		}
		return center.x + radius >= aCascade.MinX && center.x - radius <= aCascade.MaxX && center.y + radius >= aCascade.MinY &&
		       center.y - radius <= aCascade.MaxY && center.z + radius >= aCascade.MinZ && center.z - radius <= aCascade.MaxZ;
	}

	bool IntersectsPointLightRadius(const GraphicsEngine::LightSnapshot& aLight, const GraphicsEngine::RenderItemSnapshot& aRenderItem)
	{
		if (!aRenderItem.HasBounds || !IsFinite(aLight.Position) || !IsFinite(aRenderItem.BoundsCenter) ||
		    !std::isfinite(aRenderItem.BoundsRadius) || aRenderItem.BoundsRadius < 0.0f || aLight.Radius <= 0.0f ||
		    !std::isfinite(aLight.Radius))
		{
			return true;
		}

		const float radius = aLight.Radius + aRenderItem.BoundsRadius;
		return (aRenderItem.BoundsCenter - aLight.Position).LengthSqr() <= radius * radius;
	}

	RenderItemPtrList CullCastersForCascade(const std::vector<GraphicsEngine::RenderItemSnapshot>& aRenderItems,
	                                        const CascadeShadowData& aCascade)
	{
		RenderItemPtrList visibleCasters;
		visibleCasters.reserve(aRenderItems.size());
		for (const GraphicsEngine::RenderItemSnapshot& item : aRenderItems)
		{
			if (IntersectsLightSpaceBounds(aCascade, item))
			{
				visibleCasters.emplace_back(&item);
			}
		}
		return visibleCasters;
	}

	RenderItemPtrList CullCastersForFrustum(const std::vector<GraphicsEngine::RenderItemSnapshot>& aRenderItems,
	                                        const CameraFrustum& aFrustum)
	{
		RenderItemPtrList visibleCasters;
		visibleCasters.reserve(aRenderItems.size());
		for (const GraphicsEngine::RenderItemSnapshot& item : aRenderItems)
		{
			if (!item.HasBounds || IntersectsFrustum(aFrustum, {item.BoundsCenter, item.BoundsRadius, item.HasBounds}))
			{
				visibleCasters.emplace_back(&item);
			}
		}
		return visibleCasters;
	}

	RenderItemPtrList CullCastersForPointLight(const std::vector<GraphicsEngine::RenderItemSnapshot>& aRenderItems,
	                                           const GraphicsEngine::LightSnapshot& aLight)
	{
		RenderItemPtrList visibleCasters;
		visibleCasters.reserve(aRenderItems.size());
		for (const GraphicsEngine::RenderItemSnapshot& item : aRenderItems)
		{
			if (IntersectsPointLightRadius(aLight, item))
			{
				visibleCasters.emplace_back(&item);
			}
		}
		return visibleCasters;
	}

	float GetRenderIntensity(const GraphicsEngine::LightSnapshot& aLight)
	{
		if (aLight.Type == RenderLightType::Directional)
		{
			return aLight.Intensity;
		}

		return aLight.Intensity * 10000.0f;
	}

	BlendMode GetElementBlendMode(const Mesh::Element& anElement, const std::vector<std::shared_ptr<MaterialInterface>>& someMaterials,
	                              const MaterialInterface& aFallbackMaterial)
	{
		if (anElement.MaterialIndex < someMaterials.size() && someMaterials[anElement.MaterialIndex] != nullptr)
		{
			return someMaterials[anElement.MaterialIndex]->GetBlendMode();
		}

		return aFallbackMaterial.GetBlendMode();
	}

	CU::Matrix4f CreateNDCToTextureMatrix()
	{
		return {0.5f, 0.0f, 0.0f, 0.0f, 0.0f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 0.5f, 0.0f, 1.0f};
	}

	CU::Vector3f GetLightUpVector(const CU::Vector3f& aDirection)
	{
		const float yAlignment = std::abs(aDirection.Dot(CU::Vector3f::UnitY));
		return yAlignment > 0.95f ? CU::Vector3f::UnitZ : CU::Vector3f::UnitY;
	}

	std::array<CU::Vector3f, 8> GetFrustumCorners(const CU::Camera3D& aCamera, float aNearPlane, float aFarPlane)
	{
		const CU::Vector3f position = aCamera.GetPosition();
		const CU::Vector3f forward = aCamera.GetForward().GetNormalized();
		const CU::Vector3f right = aCamera.GetRight().GetNormalized();
		const CU::Vector3f up = aCamera.GetUp().GetNormalized();
		const float tanHalfFov = std::tan(aCamera.GetFieldOfViewRadians() * 0.5f);

		const float nearHeight = 2.0f * tanHalfFov * aNearPlane;
		const float nearWidth = nearHeight * aCamera.GetAspectRatio();
		const float farHeight = 2.0f * tanHalfFov * aFarPlane;
		const float farWidth = farHeight * aCamera.GetAspectRatio();

		const CU::Vector3f nearCenter = position + forward * aNearPlane;
		const CU::Vector3f farCenter = position + forward * aFarPlane;

		return {nearCenter - right * (nearWidth * 0.5f) + up * (nearHeight * 0.5f),
		        nearCenter + right * (nearWidth * 0.5f) + up * (nearHeight * 0.5f),
		        nearCenter + right * (nearWidth * 0.5f) - up * (nearHeight * 0.5f),
		        nearCenter - right * (nearWidth * 0.5f) - up * (nearHeight * 0.5f),
		        farCenter - right * (farWidth * 0.5f) + up * (farHeight * 0.5f),
		        farCenter + right * (farWidth * 0.5f) + up * (farHeight * 0.5f),
		        farCenter + right * (farWidth * 0.5f) - up * (farHeight * 0.5f),
		        farCenter - right * (farWidth * 0.5f) - up * (farHeight * 0.5f)};
	}

	CascadeShadowData CreateCascadeShadowData(const CU::Camera3D& aCamera, const GraphicsEngine::LightSnapshot& aLight, float aNearPlane,
	                                          float aFarPlane)
	{
		const std::array<CU::Vector3f, 8> corners = GetFrustumCorners(aCamera, aNearPlane, aFarPlane);

		CU::Vector3f center = CU::Vector3f::Zero;
		for (const CU::Vector3f& corner : corners)
		{
			center += corner;
		}
		center *= 1.0f / static_cast<float>(corners.size());

		float radius = 0.0f;
		for (const CU::Vector3f& corner : corners)
		{
			radius = (std::max)(radius, (corner - center).Length());
		}
		radius = std::ceil(radius / 10.0f) * 10.0f;

		const CU::Vector3f lightDirection = aLight.Direction.GetNormalized();
		const CU::Vector3f eye = center - lightDirection * (radius + 250.0f);
		const CU::Matrix4f view = CU::Maths::CreateLookAtLH(eye, center, GetLightUpVector(lightDirection));

		float minX = (std::numeric_limits<float>::max)();
		float minY = (std::numeric_limits<float>::max)();
		float minZ = (std::numeric_limits<float>::max)();
		float maxX = (std::numeric_limits<float>::lowest)();
		float maxY = (std::numeric_limits<float>::lowest)();
		float maxZ = (std::numeric_limits<float>::lowest)();

		for (const CU::Vector3f& corner : corners)
		{
			const CU::Vector3f lightSpaceCorner = CU::Maths::TransformPoint(corner, view);
			minX = (std::min)(minX, lightSpaceCorner.x);
			minY = (std::min)(minY, lightSpaceCorner.y);
			minZ = (std::min)(minZ, lightSpaceCorner.z);
			maxX = (std::max)(maxX, lightSpaceCorner.x);
			maxY = (std::max)(maxY, lightSpaceCorner.y);
			maxZ = (std::max)(maxZ, lightSpaceCorner.z);
		}

		const float xyPadding =
		    (std::max)(ShadowConfig::DirectionalCascadeLightPaddingMin, radius * ShadowConfig::DirectionalCascadeLightPaddingScale);
		const float zPadding =
		    (std::max)(ShadowConfig::DirectionalCascadeLightPaddingMin, radius * ShadowConfig::DirectionalCascadeDepthPaddingScale);

		minX -= xyPadding;
		maxX += xyPadding;
		minY -= xyPadding;
		maxY += xyPadding;
		minZ = (std::max)(0.1f, minZ - zPadding);
		maxZ += zPadding;

		const float width = maxX - minX;
		const float height = maxY - minY;
		const float texelSizeX = width / static_cast<float>(ShadowConfig::MapResolution);
		const float texelSizeY = height / static_cast<float>(ShadowConfig::MapResolution);
		minX = std::floor(minX / texelSizeX) * texelSizeX;
		minY = std::floor(minY / texelSizeY) * texelSizeY;
		maxX = minX + width;
		maxY = minY + height;

		const CU::Matrix4f projection = CU::Maths::CreateOrthographicLH(minX, maxX, minY, maxY, minZ, maxZ);

		CascadeShadowData data;
		data.View = view;
		data.ViewProjection = view * projection;
		data.MinX = minX;
		data.MaxX = maxX;
		data.MinY = minY;
		data.MaxY = maxY;
		data.MinZ = minZ;
		data.MaxZ = maxZ;
		data.DepthRange = (std::max)(maxZ - minZ, 1.0f);
		data.HasBounds = std::isfinite(minX) && std::isfinite(maxX) && std::isfinite(minY) && std::isfinite(maxY) && std::isfinite(minZ) &&
		                 std::isfinite(maxZ) && minX < maxX && minY < maxY && minZ < maxZ;
		return data;
	}

	CU::Matrix4f CreateLightViewProjectionTexture(const CU::Matrix4f& aViewProjection)
	{
		return aViewProjection * CreateNDCToTextureMatrix();
	}

	CU::Matrix4f CreateSpotViewProjection(const GraphicsEngine::LightSnapshot& aLight)
	{
		const CU::Vector3f position = aLight.Position;
		const CU::Vector3f direction = aLight.Direction.GetNormalized();
		const CU::Matrix4f view = CU::Maths::CreateLookAtLH(position, position + direction, GetLightUpVector(direction));
		const CU::Matrix4f projection = CU::Maths::CreatePerspectiveFovLH(aLight.OuterCone * 2.0f, 1.0f, 1.0f, aLight.Radius);
		return view * projection;
	}

	PointShadowBufferData CreatePointShadowBuffer(const GraphicsEngine::LightSnapshot& aLight)
	{
		const CU::Vector3f position = aLight.Position;
		const CU::Matrix4f projection = CU::Maths::CreatePerspectiveFovLH(CU::Maths::HalfPi<float>(), 1.0f, 1.0f, aLight.Radius);

		const std::array<CU::Vector3f, 6> directions = {CU::Vector3f::UnitX,  -CU::Vector3f::UnitX, CU::Vector3f::UnitY,
		                                                -CU::Vector3f::UnitY, CU::Vector3f::UnitZ,  -CU::Vector3f::UnitZ};

		const std::array<CU::Vector3f, 6> upVectors = {CU::Vector3f::UnitY, CU::Vector3f::UnitY, -CU::Vector3f::UnitZ,
		                                               CU::Vector3f::UnitZ, CU::Vector3f::UnitY, CU::Vector3f::UnitY};

		PointShadowBufferData buffer;
		for (size_t face = 0; face < buffer.ViewProjection.size(); ++face)
		{
			buffer.ViewProjection[face] = CU::Maths::CreateLookAtLH(position, position + directions[face], upVectors[face]) * projection;
		}

		return buffer;
	}

	CU::Vector4f MakeShadowSettings(float aDepthBias)
	{
		return {aDepthBias, 0.0f, 0.0f, 0.0f};
	}

	LightBuffer::Light* AddLightToBuffer(const GraphicsEngine::LightSnapshot& aLight, LightBuffer& inoutLightBuffer, float aShadowDepthBias)
	{
		if (inoutLightBuffer.NumActiveLights >= LightBuffer::MaxLights)
		{
			return nullptr;
		}

		LightBuffer::Light& light = inoutLightBuffer.Lights[inoutLightBuffer.NumActiveLights++];
		light.Color = aLight.Color;
		light.Intensity = GetRenderIntensity(aLight);
		light.Position = aLight.Position;
		light.Type = static_cast<unsigned>(aLight.Type);
		light.Direction = aLight.Direction;
		light.InnerCone = aLight.InnerCone;
		light.OuterCone = aLight.OuterCone;
		light.Radius = aLight.Radius;
		light.ShadowMapIndex = 0;
		light.NumCascades = 0;
		light.CascadeSplits = CU::Vector4f::Zero;
		light.ShadowSettings = MakeShadowSettings(aShadowDepthBias);
		light.CascadeDepthBiases = CU::Vector4f::Zero;
		light.CascadeFilterWorldRadii = CU::Vector4f::Zero;
		return &light;
	}

	bool IsRelevantLight(const CameraFrustum& aFrustum, const GraphicsEngine::LightSnapshot& aLight)
	{
		if (aLight.Type == RenderLightType::Directional)
		{
			return true;
		}

		return IntersectsFrustum(aFrustum, {aLight.Position, aLight.Radius, true});
	}

}

struct GraphicsEngine::ShadowRenderJob
{
	std::string EventName;
	Texture* ShadowMap = nullptr;
	FrameBuffer FrameBufferData;
	const PipelineStateObject* OverridePSO = nullptr;
	PipeLineStages OverrideStages = PipeLineStage_None;
	PointShadowBufferData PointShadowBuffer;
	bool HasPointShadowBuffer = false;
	RenderItemPtrList RenderItems;
};

// --- Snapshot storage ---

void GraphicsEngine::RenderSceneSnapshot::Clear()
{
	HasCamera = false;
	ShadowCasters.clear();
	OpaqueRenderItems.clear();
	BlendedRenderItems.clear();
	RelevantLights.clear();
	Stats = {};
}

GraphicsEngine& GraphicsEngine::Get()
{
	static GraphicsEngine myInstance;
	return myInstance;
}

// --- Initialization ---

bool GraphicsEngine::Initialize(HWND aWindowHandle, const std::filesystem::path& aShaderRoot)
{
	if (!std::filesystem::exists(aShaderRoot))
	{
		GELOG(Error, "Shader root dir is not a valid path! Provided root was {}.", aShaderRoot.string());
		return false;
	}

	myCullingEnabled = !StartupOptions::Disabled(L"AGP_DISABLE_CULLING");
	myParallelShadowsEnabled = !StartupOptions::Disabled(L"AGP_DISABLE_PARALLEL_SHADOWS");
	myShaderRoot = aShaderRoot;

	if (!myRHI.Initialize(aWindowHandle, true, myBackBuffer, myDepthBuffer))
	{
		return false; // RHI logs this for us
	}

	if (!CreateDefaultTextures())
	{
		return false;
	}

	if (!CreateGBufferResources() || !CreateDeferredPipelineStates())
	{
		return false;
	}

	myMaterialDomainShaders.emplace(MaterialDomain::Surface, aShaderRoot / "Material" / "Surface_VS.hlsl");
	myMaterialShadingModelShaders.emplace(ShadingModel::Unlit, aShaderRoot / "Material" / "Unlit_PS.hlsl");
	myMaterialShadingModelShaders.emplace(ShadingModel::Lit, aShaderRoot / "Material" / "Lit_PS.hlsl");

	MaterialDescription defaultMaterialDesc;
	defaultMaterialDesc.Name = "Default";
	defaultMaterialDesc.BlendMode = BlendMode::Opaque;
	defaultMaterialDesc.Domain = MaterialDomain::Surface;
	defaultMaterialDesc.ShadingModel = ShadingModel::Unlit;
	defaultMaterialDesc.MaterialShaderCode = aShaderRoot / "Material" / "Material.hlsli";
	if (!CreateMaterial(defaultMaterialDesc, myDefaultMaterial))
	{
		GELOG(Error, "Failed to create default material!");
		return false;
	}

	CreateConstantBuffer<FrameBuffer>(ConstantBuffer::FrameBuffer, "FrameBuffer");
	CreateConstantBuffer<ObjectBuffer>(ConstantBuffer::ObjectBuffer, "ObjectBuffer");
	CreateConstantBuffer<AnimationBuffer>(ConstantBuffer::AnimationBuffer, "AnimationBuffer");
	CreateConstantBuffer(ConstantBuffer::MaterialBuffer, "MaterialBuffer", Material::MATERIAL_BUFFER_SIZE);
	CreateConstantBuffer<LightBuffer>(ConstantBuffer::LightBuffer, "LightBuffer");
	CreateConstantBuffer(ConstantBuffer::PointShadowBuffer, "PointShadowBuffer", sizeof(PointShadowBufferData));
	CreateConstantBuffer(ConstantBuffer::RenderPassDebugBuffer, "RenderPassDebugBuffer", 16);

	mySamplers.reserve(4);
	{ // Trilinear Wrap
		SamplerDescription samplerDesc;
		samplerDesc.Name = "TrilinearWrap";
		samplerDesc.AddressMode = SamplerAddressMode::Wrap;
		samplerDesc.FilterMode = SamplerFilterMode::Trilinear;
		Sampler sampler;
		ensure(myRHI.CreateSampler(samplerDesc, sampler));
		mySamplers.emplace_back(std::move(sampler));
	}

	{ // Shadow comparison sampler
		SamplerDescription samplerDesc;
		samplerDesc.Name = "ShadowCmpSampler";
		samplerDesc.AddressMode = SamplerAddressMode::Border;
		samplerDesc.FilterMode = SamplerFilterMode::ComparisonLinearPoint;
		samplerDesc.ComparisonFunction = SamplerComparisonFunc::LessEqual;
		samplerDesc.BorderColor = CU::Vector4f::One;
		Sampler sampler;
		ensure(myRHI.CreateSampler(samplerDesc, sampler));
		mySamplers.emplace_back(std::move(sampler));
	}

	{ // Linear Clamp
		SamplerDescription samplerDesc;
		samplerDesc.Name = "LUTSampler";
		samplerDesc.AddressMode = SamplerAddressMode::Clamp;
		samplerDesc.FilterMode = SamplerFilterMode::Linear;
		Sampler sampler;
		ensure(myRHI.CreateSampler(samplerDesc, sampler));
		mySamplers.emplace_back(std::move(sampler));
	}

	{ // Trilinear Clamp, used for fullscreen GBuffer reads.
		SamplerDescription samplerDesc;
		samplerDesc.Name = "TrilinearClamp";
		samplerDesc.AddressMode = SamplerAddressMode::Clamp;
		samplerDesc.FilterMode = SamplerFilterMode::Trilinear;
		Sampler sampler;
		ensure(myRHI.CreateSampler(samplerDesc, sampler));
		mySamplers.emplace_back(std::move(sampler));
	}

	mySamplerBindings.clear();
	mySamplerBindings.reserve(mySamplers.size());
	for (const Sampler& sampler : mySamplers)
	{
		mySamplerBindings.emplace_back(&sampler);
	}

	if (!CreateShadowResources())
	{
		GELOG(Error, "Failed to create shadow resources.");
		return false;
	}

	if (!CreateShadowPipelineStates())
	{
		GELOG(Error, "Failed to create shadow pipeline states.");
		return false;
	}

	if (!CreatePBLResources())
	{
		GELOG(Error, "Failed to create PBL resources.");
		return false;
	}

	myConstantBuffersFrozen = true;
	return true;
}

// --- Frame rendering ---

void GraphicsEngine::FinalizeRenderSnapshot(RenderSceneSnapshot& snapshot) const
{
	const auto snapshotStart = Clock::now();
	if (!snapshot.HasCamera)
	{
		return;
	}
	const CameraFrustum cameraFrustum = CreateCameraFrustum(snapshot.Camera);
	snapshot.Stats.TotalLights = static_cast<uint32_t>(snapshot.RelevantLights.size());
	if (myCullingEnabled)
	{
		std::erase_if(snapshot.RelevantLights, [&cameraFrustum](const LightSnapshot& light)
		{
			return !IsRelevantLight(cameraFrustum, light);
		});
	}
	snapshot.Stats.RelevantLights = static_cast<uint32_t>(snapshot.RelevantLights.size());
	snapshot.OpaqueRenderItems.clear();
	snapshot.BlendedRenderItems.clear();
	snapshot.OpaqueRenderItems.reserve(snapshot.ShadowCasters.size());
	snapshot.BlendedRenderItems.reserve(snapshot.ShadowCasters.size());
	snapshot.Stats.TotalRenderItems = static_cast<uint32_t>(snapshot.ShadowCasters.size());
	snapshot.Stats.VisibleRenderItems = 0;
	for (size_t itemIndex = 0; itemIndex < snapshot.ShadowCasters.size(); ++itemIndex)
	{
		auto& renderItem = snapshot.ShadowCasters[itemIndex];
		const auto& mesh = renderItem.Mesh;
		if (!mesh)
		{
			continue;
		}
		const BoundingSphere worldBounds =
		    TransformBoundingSphere(mesh->myLocalBoundsCenter, mesh->myLocalBoundsRadius, mesh->myHasLocalBounds, renderItem.World);
		renderItem.HasBounds = myCullingEnabled && worldBounds.IsValid && !renderItem.HasSkinning;
		renderItem.BoundsCenter = worldBounds.Center;
		renderItem.BoundsRadius = worldBounds.Radius;
		if (!renderItem.HasBounds || IntersectsFrustum(cameraFrustum, worldBounds))
		{
			++snapshot.Stats.VisibleRenderItems;
			const auto passes = RenderItemRouting::Classify(mesh->GetElements(), [this, &renderItem](const Mesh::Element& element)
			{
				return GetElementBlendMode(element, renderItem.Materials, myDefaultMaterial) == BlendMode::Opaque;
			});
			if (passes.Opaque)
			{
				snapshot.OpaqueRenderItems.push_back(itemIndex);
			}
			if (passes.Blended)
			{
				snapshot.BlendedRenderItems.push_back(itemIndex);
			}
		}
	}
	snapshot.Stats.ShadowCasters = static_cast<uint32_t>(snapshot.ShadowCasters.size());
	snapshot.Stats.OpaqueRenderItems = static_cast<uint32_t>(snapshot.OpaqueRenderItems.size());
	snapshot.Stats.BlendedRenderItems = static_cast<uint32_t>(snapshot.BlendedRenderItems.size());
	const CU::Vector3f cameraPosition = snapshot.Camera.GetPosition();
	auto distance = [&snapshot, cameraPosition](size_t index)
	{
		const auto& world = snapshot.ShadowCasters[index].World;
		const float d = (CU::Vector3f(world(4, 1), world(4, 2), world(4, 3)) - cameraPosition).LengthSqr();
		return std::isfinite(d) ? d : 0.0f;
	};
	RenderItemRouting::Sort(snapshot.OpaqueRenderItems, snapshot.BlendedRenderItems, distance);
	snapshot.Stats.SnapshotMilliseconds = ElapsedMilliseconds(snapshotStart);
}

void GraphicsEngine::RenderSnapshot(GraphicsCommandList& inoutCommandList, const RenderSceneSnapshot& aSnapshot)
{
	if (!aSnapshot.HasCamera)
	{
		// An empty presentation is still a completed frame. Clear the swapchain
		// target so a removed/disabled camera cannot retain a stale scene image.
		inoutCommandList.ClearOverridePipelineState();
		inoutCommandList.ClearRenderTarget(myBackBuffer);
		inoutCommandList.ClearDepthStencil(myDepthBuffer);
		inoutCommandList.SetRenderTarget(&myBackBuffer, &myDepthBuffer);
		StoreLastRenderStats(aSnapshot.Stats);
		return;
	}

	// Finish shared resource mutations before launching shadow workers.
	const auto preparationStart = Clock::now();
	PrepareSnapshotRenderResources(aSnapshot);
	UnbindShadowResources(inoutCommandList);
	RenderStats frameStats = aSnapshot.Stats;
	frameStats.ResourcePreparationMilliseconds = ElapsedMilliseconds(preparationStart);

	// Jobs borrow snapshot items; recording joins every worker before returning.
	const auto shadowStart = Clock::now();
	LightBuffer lightBuffer;
	const auto shadowJobs = BuildShadowJobs(aSnapshot, lightBuffer, frameStats);
	RecordAndExecuteShadows(inoutCommandList, shadowJobs, frameStats);
	frameStats.ShadowRecordingMilliseconds = ElapsedMilliseconds(shadowStart);

	const auto sceneStart = Clock::now();
	PrepareSceneCommands(inoutCommandList, aSnapshot);
	const auto& gbufferTextures = myGBuffer.GetTextures();
	const std::array<const Texture*, GBuffer::TargetCount> gbufferTargets = {
	    &gbufferTextures[GBuffer::Albedo], &gbufferTextures[GBuffer::PixelNormal], &gbufferTextures[GBuffer::Surface],
	    &gbufferTextures[GBuffer::Emission], &gbufferTextures[GBuffer::WorldPosition]};
	RenderGBuffer(inoutCommandList, aSnapshot, gbufferTargets);
	RenderAmbientOcclusion(inoutCommandList, gbufferTargets);
	RenderDeferredLighting(inoutCommandList, lightBuffer, gbufferTargets);
	RenderDebugView(inoutCommandList, lightBuffer, gbufferTargets);
	RenderTransparentGeometry(inoutCommandList, aSnapshot, lightBuffer);
	frameStats.SceneRecordingMilliseconds = ElapsedMilliseconds(sceneStart);
	StoreLastRenderStats(frameStats);
}

std::vector<GraphicsEngine::ShadowRenderJob> GraphicsEngine::BuildShadowJobs(const RenderSceneSnapshot& aSnapshot, LightBuffer& lightBuffer,
                                                                             RenderStats& frameStats)
{
	bool hasRenderedDirectionalShadow = false;
	unsigned spotShadowCount = 0;
	unsigned pointShadowCount = 0;
	std::vector<ShadowRenderJob> shadowJobs;
	shadowJobs.reserve(ShadowConfig::DirectionalCascadeCount + ShadowConfig::MaxSpotMaps + ShadowConfig::MaxPointMaps);

	auto trackCasterList = [&frameStats, totalCasters = aSnapshot.ShadowCasters.size()](const RenderItemPtrList& aRenderItems)
	{
		frameStats.ShadowCasterDraws += static_cast<uint32_t>(aRenderItems.size());
		if (totalCasters > aRenderItems.size())
		{
			frameStats.CulledShadowCasters += static_cast<uint32_t>(totalCasters - aRenderItems.size());
		}
	};

	for (const LightSnapshot& lightSnapshot : aSnapshot.RelevantLights)
	{
		LightBuffer::Light* light = AddLightToBuffer(lightSnapshot, lightBuffer, GetShadowDepthBias(lightSnapshot.Type));
		if (light == nullptr)
		{
			break;
		}

		if (lightSnapshot.Type == RenderLightType::Directional && !hasRenderedDirectionalShadow)
		{
			float cascadeNear = aSnapshot.Camera.GetNearPlane();
			std::array<CascadeShadowData, ShadowConfig::DirectionalCascadeCount> cascadeData = {};
			for (unsigned cascadeIndex = 0; cascadeIndex < ShadowConfig::DirectionalCascadeCount; ++cascadeIndex)
			{
				const float cascadeFar = ShadowConfig::CascadeSplits[cascadeIndex];
				const float cascadeLength = cascadeFar - cascadeNear;
				const float cascadePadding = (std::max)(ShadowConfig::DirectionalCascadeSplitPaddingMin,
				                                        cascadeLength * ShadowConfig::DirectionalCascadeSplitPaddingScale);
				const float fitNear = (std::max)(aSnapshot.Camera.GetNearPlane(), cascadeNear - cascadePadding);
				const float fitFar = cascadeFar + cascadePadding;
				cascadeData[cascadeIndex] = CreateCascadeShadowData(aSnapshot.Camera, lightSnapshot, fitNear, fitFar);

				FrameBuffer shadowFrameBuffer;
				shadowFrameBuffer.View = CU::Matrix4f();
				shadowFrameBuffer.Projection = cascadeData[cascadeIndex].ViewProjection;

				ShadowRenderJob job;
				job.EventName = std::format("Directional Shadow Cascade {}", cascadeIndex);
				job.ShadowMap = &myDirectionalShadowMaps[cascadeIndex];
				job.FrameBufferData = shadowFrameBuffer;
				job.OverridePSO = &myShadowOverridePSO;
				job.OverrideStages = PipeLineStage_PixelShader | PipeLineStage_Rasterizer;
				job.RenderItems = CullCastersForCascade(aSnapshot.ShadowCasters, cascadeData[cascadeIndex]);
				trackCasterList(job.RenderItems);
				shadowJobs.emplace_back(std::move(job));
				++frameStats.DirectionalShadowPasses;

				light->LightViewProjTexture[cascadeIndex] = CreateLightViewProjectionTexture(cascadeData[cascadeIndex].ViewProjection);
				cascadeNear = cascadeFar;
			}

			const float baseDirectionalBias = GetShadowDepthBias(RenderLightType::Directional);
			const float referenceDepthRange = cascadeData[0].DepthRange;
			light->NumCascades = ShadowConfig::DirectionalCascadeCount;
			light->CascadeSplits = {ShadowConfig::CascadeSplits[0], ShadowConfig::CascadeSplits[1], ShadowConfig::CascadeSplits[2],
			                        ShadowConfig::CascadeSplits[3]};
			light->ShadowSettings = MakeShadowSettings(GetShadowDepthBias(RenderLightType::Directional));
			light->CascadeDepthBiases = {baseDirectionalBias * referenceDepthRange / cascadeData[0].DepthRange,
			                             baseDirectionalBias * referenceDepthRange / cascadeData[1].DepthRange,
			                             baseDirectionalBias * referenceDepthRange / cascadeData[2].DepthRange,
			                             baseDirectionalBias * referenceDepthRange / cascadeData[3].DepthRange};
			light->CascadeFilterWorldRadii = {ShadowConfig::DirectionalFilterRadiusWorld, ShadowConfig::DirectionalFilterRadiusWorld,
			                                  ShadowConfig::DirectionalFilterRadiusWorld, ShadowConfig::DirectionalFilterRadiusWorld};
			hasRenderedDirectionalShadow = true;
		}
		else if (lightSnapshot.Type == RenderLightType::Spot && spotShadowCount < MaxSpotShadowMaps)
		{
			const CU::Matrix4f lightViewProjection = CreateSpotViewProjection(lightSnapshot);
			FrameBuffer shadowFrameBuffer;
			shadowFrameBuffer.View = CU::Matrix4f();
			shadowFrameBuffer.Projection = lightViewProjection;

			ShadowRenderJob job;
			job.EventName = std::format("Spot Shadow {}", spotShadowCount);
			job.ShadowMap = &mySpotShadowMaps[spotShadowCount];
			job.FrameBufferData = shadowFrameBuffer;
			job.OverridePSO = &myLocalShadowOverridePSO;
			job.OverrideStages = PipeLineStage_PixelShader | PipeLineStage_Rasterizer;
			job.RenderItems = CullCastersForFrustum(aSnapshot.ShadowCasters, CreateFrustumFromViewProjection(lightViewProjection));
			trackCasterList(job.RenderItems);
			shadowJobs.emplace_back(std::move(job));
			++frameStats.SpotShadowPasses;

			light->ShadowMapIndex = spotShadowCount;
			light->NumCascades = 1;
			light->LightViewProjTexture[0] = CreateLightViewProjectionTexture(lightViewProjection);
			light->ShadowSettings = MakeShadowSettings(GetShadowDepthBias(RenderLightType::Spot));
			++spotShadowCount;
		}
		else if (lightSnapshot.Type == RenderLightType::Point && pointShadowCount < MaxPointShadowMaps)
		{
			const PointShadowBufferData pointShadowBuffer = CreatePointShadowBuffer(lightSnapshot);
			FrameBuffer shadowFrameBuffer;
			shadowFrameBuffer.View = CU::Matrix4f();
			shadowFrameBuffer.Projection = CU::Matrix4f();

			ShadowRenderJob job;
			job.EventName = std::format("Point Shadow {}", pointShadowCount);
			job.ShadowMap = &myPointShadowMaps[pointShadowCount];
			job.FrameBufferData = shadowFrameBuffer;
			job.OverridePSO = &myPointShadowOverridePSO;
			job.OverrideStages = PipeLineStage_PixelShader | PipeLineStage_Rasterizer | PipeLineStage_GeometryShader;
			job.PointShadowBuffer = pointShadowBuffer;
			job.HasPointShadowBuffer = true;
			job.RenderItems = CullCastersForPointLight(aSnapshot.ShadowCasters, lightSnapshot);
			trackCasterList(job.RenderItems);
			shadowJobs.emplace_back(std::move(job));
			++frameStats.PointShadowPasses;

			light->ShadowMapIndex = pointShadowCount;
			light->NumCascades = 1;
			light->ShadowSettings = MakeShadowSettings(GetShadowDepthBias(RenderLightType::Point));
			++pointShadowCount;
		}
	}

	return shadowJobs;
}

void GraphicsEngine::RecordAndExecuteShadows(GraphicsCommandList& inoutCommandList, const std::vector<ShadowRenderJob>& shadowJobs,
                                             RenderStats& frameStats)
{
	auto recordShadowJob = [this](GraphicsCommandList& inoutShadowCommandList, const ShadowRenderJob& aJob, bool aFinishCommandList)
	{
		ensure(aJob.ShadowMap != nullptr);
		ensure(aJob.OverridePSO != nullptr);
		RenderShadowMap(inoutShadowCommandList, aJob.EventName, *aJob.ShadowMap, aJob.FrameBufferData, *aJob.OverridePSO,
		                aJob.OverrideStages, aJob.HasPointShadowBuffer ? &aJob.PointShadowBuffer : nullptr, aJob.RenderItems);

		if (aFinishCommandList)
		{
			return inoutShadowCommandList.FinishCommandList();
		}
		return true;
	};

	if (!shadowJobs.empty())
	{
		bool recorded = false;
		if (myParallelShadowsEnabled && EnsureShadowCommandListCount(shadowJobs.size()))
		{
			std::vector<std::future<bool>> futures;
			futures.reserve(shadowJobs.size());
			recorded = true;
			try
			{
				for (size_t i = 0; i < shadowJobs.size(); ++i)
				{
					auto& commandList = myShadowCommandLists[i];
					commandList.ResetCommandList();
					futures.emplace_back(std::async(std::launch::async, [this, &recordShadowJob, &shadowJobs, i]
					{
						return recordShadowJob(myShadowCommandLists[i], shadowJobs[i], true);
					}));
				}
			}
			catch (...)
			{
				recorded = false;
			}
			const auto waitStart = Clock::now();
			// Join every worker before fallback, playback, or destroying job data.
			for (auto& future : futures)
			{
				try
				{
					if (!future.get())
					{
						recorded = false;
					}
				}
				catch (...)
				{
					recorded = false;
				}
			}
			frameStats.ShadowWaitMilliseconds = ElapsedMilliseconds(waitStart);
			if (recorded)
			{
				for (size_t i = 0; i < shadowJobs.size(); ++i)
				{
					ExecuteCommandList(myShadowCommandLists[i]);
				}
				frameStats.ShadowCommandListsRecorded = static_cast<uint32_t>(shadowJobs.size());
				frameStats.ShadowCommandListsExecuted = static_cast<uint32_t>(shadowJobs.size());
			}
			else
			{
				GELOG(Warning, "Parallel shadow recording failed; retrying all shadows serially.");
				myShadowCommandLists.clear();
			}
		}
		if (!recorded)
		{
			for (const auto& job : shadowJobs)
			{
				recordShadowJob(inoutCommandList, job, false);
			}
		}
	}
}

void GraphicsEngine::PrepareSceneCommands(GraphicsCommandList& inoutCommandList, const RenderSceneSnapshot& aSnapshot)
{
	inoutCommandList.ClearOverridePipelineState();

	inoutCommandList.ClearRenderTarget(myBackBuffer);
	inoutCommandList.ClearDepthStencil(myDepthBuffer);
	inoutCommandList.SetRenderTarget(&myBackBuffer, &myDepthBuffer);

	inoutCommandList.SetShaderSamplers(mySamplerBindings.data(), mySamplerBindings.size(), 0,
	                                   PipeLineStage_VertexShader | PipeLineStage_PixelShader);
	BindPBLResources(inoutCommandList);
	BindShadowResources(inoutCommandList);

	FrameBuffer fb;
	fb.View = aSnapshot.Camera.GetViewMatrix();
	fb.Projection = aSnapshot.Camera.GetProjectionMatrix();
	const CU::Vector3f cameraPosition = aSnapshot.Camera.GetPosition();
	fb.CameraPosition = {cameraPosition.x, cameraPosition.y, cameraPosition.z, 1.0f};

	UpdateAndSetConstantBuffer(inoutCommandList, ConstantBuffer::FrameBuffer, fb, 0,
	                           PipeLineStage_VertexShader | PipeLineStage_PixelShader);
}

void GraphicsEngine::RenderGBuffer(GraphicsCommandList& inoutCommandList, const RenderSceneSnapshot& aSnapshot,
                                   const GBufferBindings& gbufferTargets)
{
	// --- Deferred GBuffer ---
	// Writes surface data and depth for SSAO, lighting, and forward transparency.
	// Non-blended geometry produces the five ordered GBuffer targets. The sampled
	// tangent normal is captured separately only for its dedicated debug view.
	inoutCommandList.BeginEvent("Deferred GBuffer");
	const std::array<Texture, GBuffer::TargetCount>& gbufferTextures = myGBuffer.GetTextures();
	for (const Texture& target : gbufferTextures)
	{
		inoutCommandList.ClearRenderTarget(target);
	}
	const bool captureTangentNormals = myRenderPass == RenderPass::NormalsTangentSpace;
	std::array<const Texture*, GBuffer::TargetCount + 1> gbufferRenderTargets = {};
	std::copy(gbufferTargets.begin(), gbufferTargets.end(), gbufferRenderTargets.begin());
	if (captureTangentNormals)
	{
		inoutCommandList.ClearRenderTarget(myTangentNormalDebugTexture);
		gbufferRenderTargets[GBuffer::TargetCount] = &myTangentNormalDebugTexture;
	}
	inoutCommandList.SetRenderTargets(gbufferRenderTargets.data(),
	                                  captureTangentNormals ? gbufferRenderTargets.size() : gbufferTargets.size(), &myDepthBuffer);
	for (size_t itemIndex : aSnapshot.OpaqueRenderItems)
	{
		RenderMesh(inoutCommandList, aSnapshot.ShadowCasters[itemIndex], false, RenderBlendFilter::OpaqueOnly, true);
	}
	inoutCommandList.SetRenderTargets(nullptr, 0, nullptr);
	inoutCommandList.EndEvent();
}

void GraphicsEngine::RenderAmbientOcclusion(GraphicsCommandList& inoutCommandList, const GBufferBindings& gbufferTargets)
{
	// --- Screen Space Ambient Occlusion ---
	// Reads GBuffer world positions and normals after its render targets are unbound.
	// SSAO is generated from the deferred surface data so it can be inspected and
	// used independently from the material's packed texture AO channel.
	inoutCommandList.BeginEvent("Screen Space Ambient Occlusion");
	inoutCommandList.ClearRenderTarget(myScreenSpaceAOTexture);
	inoutCommandList.SetRenderTarget(&myScreenSpaceAOTexture, nullptr);
	inoutCommandList.SetShaderResources(gbufferTargets.data(), gbufferTargets.size(), 0, PipeLineStage_PixelShader);
	inoutCommandList.SetPipelineState(&myScreenSpaceAOPSO);
	inoutCommandList.Draw(4);
	const std::array<const Texture*, GBuffer::TargetCount> nullGBufferResources = {};
	inoutCommandList.SetShaderResources(nullGBufferResources.data(), nullGBufferResources.size(), 0, PipeLineStage_PixelShader);
	inoutCommandList.EndEvent();
}

void GraphicsEngine::RenderDeferredLighting(GraphicsCommandList& inoutCommandList, const LightBuffer& lightBuffer,
                                            const GBufferBindings& gbufferTargets)
{
	// --- Deferred Lighting ---
	// Reads GBuffer, SSAO, and completed shadow maps.
	// Deferred lights are fullscreen additive passes. Accumulation stays linear until
	// the final composite pass, matching the Forward shader's gamma conversion.
	inoutCommandList.BeginEvent("Deferred Lighting");
	inoutCommandList.ClearRenderTarget(myDeferredLightingTexture);
	inoutCommandList.SetRenderTarget(&myDeferredLightingTexture, nullptr);
	inoutCommandList.SetShaderResources(gbufferTargets.data(), gbufferTargets.size(), 0, PipeLineStage_PixelShader);
	const Texture* screenSpaceAOResource = &myScreenSpaceAOTexture;
	inoutCommandList.SetShaderResources(&screenSpaceAOResource, 1, GBuffer::TargetCount, PipeLineStage_PixelShader);
	for (unsigned lightIndex = 0; lightIndex < lightBuffer.NumActiveLights; ++lightIndex)
	{
		LightBuffer singleLightBuffer;
		singleLightBuffer.Lights[0] = lightBuffer.Lights[lightIndex];
		singleLightBuffer.NumActiveLights = 1;
		UpdateAndSetConstantBuffer(inoutCommandList, ConstantBuffer::LightBuffer, singleLightBuffer, 4, PipeLineStage_PixelShader);

		switch (singleLightBuffer.Lights[0].Type)
		{
		case static_cast<unsigned>(RenderLightType::Directional):
			inoutCommandList.SetPipelineState(&myDeferredDirectionalPSO);
			break;
		case static_cast<unsigned>(RenderLightType::Point):
			inoutCommandList.SetPipelineState(&myDeferredPointPSO);
			break;
		case static_cast<unsigned>(RenderLightType::Spot):
			inoutCommandList.SetPipelineState(&myDeferredSpotPSO);
			break;
		default:
			continue;
		}
		inoutCommandList.Draw(4);
	}
	const std::array<const Texture*, GBuffer::TargetCount> nullGBufferResources = {};
	inoutCommandList.SetShaderResources(nullGBufferResources.data(), nullGBufferResources.size(), 0, PipeLineStage_PixelShader);
	const Texture* nullScreenSpaceAOResource = nullptr;
	inoutCommandList.SetShaderResources(&nullScreenSpaceAOResource, 1, GBuffer::TargetCount, PipeLineStage_PixelShader);
	inoutCommandList.SetRenderTarget(&myBackBuffer, nullptr);
	const Texture* deferredLightingResource = &myDeferredLightingTexture;
	inoutCommandList.SetShaderResources(&deferredLightingResource, 1, 0, PipeLineStage_PixelShader);
	inoutCommandList.SetPipelineState(&myDeferredCompositePSO);
	inoutCommandList.Draw(4);
	const std::array<const Texture*, 1> nullDeferredLightingResource = {};
	inoutCommandList.SetShaderResources(nullDeferredLightingResource.data(), nullDeferredLightingResource.size(), 0,
	                                    PipeLineStage_PixelShader);
	inoutCommandList.EndEvent();
}

void GraphicsEngine::RenderDebugView(GraphicsCommandList& inoutCommandList, const LightBuffer& lightBuffer,
                                     const GBufferBindings& gbufferTargets)
{
	// --- Render Pass Debug ---
	// Replaces the composite with the selected diagnostic view.
	if (myRenderPass != RenderPass::Lit)
	{
		inoutCommandList.BeginEvent("Render Pass Debug");
		inoutCommandList.SetRenderTarget(&myBackBuffer, nullptr);
		inoutCommandList.SetShaderResources(gbufferTargets.data(), gbufferTargets.size(), 0, PipeLineStage_PixelShader);
		const Texture* screenSpaceAO = &myScreenSpaceAOTexture;
		inoutCommandList.SetShaderResources(&screenSpaceAO, 1, GBuffer::TargetCount, PipeLineStage_PixelShader);
		const Texture* tangentNormalDebug = &myTangentNormalDebugTexture;
		inoutCommandList.SetShaderResources(&tangentNormalDebug, 1, GBuffer::TargetCount + 1, PipeLineStage_PixelShader);
		const std::array<uint32_t, 4> renderPass = {static_cast<uint32_t>(myRenderPass), 0, 0, 0};
		UpdateAndSetConstantBuffer(inoutCommandList, ConstantBuffer::RenderPassDebugBuffer, renderPass, 5, PipeLineStage_PixelShader);
		UpdateAndSetConstantBuffer(inoutCommandList, ConstantBuffer::LightBuffer, lightBuffer, 4, PipeLineStage_PixelShader);
		inoutCommandList.SetPipelineState(&myRenderPassDebugPSO);
		inoutCommandList.Draw(4);
		const std::array<const Texture*, GBuffer::TargetCount + 2> nullDebugResources = {};
		inoutCommandList.SetShaderResources(nullDebugResources.data(), nullDebugResources.size(), 0, PipeLineStage_PixelShader);
		inoutCommandList.EndEvent();
	}
}

void GraphicsEngine::RenderTransparentGeometry(GraphicsCommandList& inoutCommandList, const RenderSceneSnapshot& aSnapshot,
                                               const LightBuffer& lightBuffer)
{
	// --- Forward transparency ---
	// Blended elements remain Forward rendered and use the depth written in GBuffer.
	inoutCommandList.SetRenderTarget(&myBackBuffer, &myDepthBuffer);
	UpdateAndSetConstantBuffer(inoutCommandList, ConstantBuffer::LightBuffer, lightBuffer, 4, PipeLineStage_PixelShader);
	for (size_t itemIndex : aSnapshot.BlendedRenderItems)
	{
		RenderMesh(inoutCommandList, aSnapshot.ShadowCasters[itemIndex], false, RenderBlendFilter::BlendedOnly);
	}
}

void GraphicsEngine::Present() const
{
	myRHI.Present();
}

// --- Diagnostics ---

void GraphicsEngine::CycleRenderPass()
{
	const auto nextPass = static_cast<uint8_t>(myRenderPass) + 1;
	myRenderPass = nextPass == static_cast<uint8_t>(RenderPass::Count) ? RenderPass::Lit : static_cast<RenderPass>(nextPass);
}

const char* GraphicsEngine::GetRenderPassName() const
{
	switch (myRenderPass)
	{
	case RenderPass::Lit:
		return "Lit";
	case RenderPass::Albedo:
		return "Albedo (sRGB)";
	case RenderPass::Roughness:
		return "Roughness (Linear Greyscale)";
	case RenderPass::Metalness:
		return "Metalness (Linear Greyscale)";
	case RenderPass::AmbientOcclusionTexture:
		return "Ambient Occlusion (Texture, Linear Greyscale)";
	case RenderPass::AmbientOcclusionScreenSpace:
		return "Ambient Occlusion (Screen Space, Linear Greyscale)";
	case RenderPass::NormalsTangentSpace:
		return "Normals (Tangent Space, Linear)";
	case RenderPass::NormalsWorldSpace:
		return "Normals (World Space, Linear)";
	case RenderPass::Shadows:
		return "Shadows (Directional)";
	default:
		return "Unknown";
	}
}

// --- Scene resource bindings ---

void GraphicsEngine::UnbindShadowResources(GraphicsCommandList& inoutCommandList) const
{
	std::array<const Texture*, ShadowConfig::DirectionalCascadeCount + ShadowConfig::MaxSpotMaps + ShadowConfig::MaxPointMaps>
	    nullShadowResources = {};
	inoutCommandList.SetShaderResources(nullShadowResources.data(), nullShadowResources.size(), ShadowConfig::HighTextureSlotStart,
	                                    PipeLineStage_PixelShader | PipeLineStage_GeometryShader);
}

void GraphicsEngine::BindPBLResources(GraphicsCommandList& inoutCommandList) const
{
	const std::array<const Texture*, 2> pblResources = {&myEnvironmentCubeTexture, &myBRDFLUTTexture};
	inoutCommandList.SetShaderResources(pblResources.data(), pblResources.size(), PBLConfig::EnvironmentCubeSlot,
	                                    PipeLineStage_PixelShader);
}

void GraphicsEngine::BindShadowResources(GraphicsCommandList& inoutCommandList) const
{
	std::array<const Texture*, ShadowConfig::DirectionalCascadeCount + ShadowConfig::MaxSpotMaps + ShadowConfig::MaxPointMaps>
	    shadowResources = {};
	for (size_t cascadeIndex = 0; cascadeIndex < myDirectionalShadowMaps.size(); ++cascadeIndex)
	{
		shadowResources[cascadeIndex] = &myDirectionalShadowMaps[cascadeIndex];
	}
	for (size_t spotIndex = 0; spotIndex < mySpotShadowMaps.size(); ++spotIndex)
	{
		shadowResources[ShadowConfig::DirectionalCascadeCount + spotIndex] = &mySpotShadowMaps[spotIndex];
	}
	for (size_t pointIndex = 0; pointIndex < myPointShadowMaps.size(); ++pointIndex)
	{
		shadowResources[ShadowConfig::DirectionalCascadeCount + ShadowConfig::MaxSpotMaps + pointIndex] = &myPointShadowMaps[pointIndex];
	}

	inoutCommandList.SetShaderResources(shadowResources.data(), shadowResources.size(), ShadowConfig::HighTextureSlotStart,
	                                    PipeLineStage_PixelShader);
}

// --- Shadow recording and tuning ---

void GraphicsEngine::RenderShadowMap(GraphicsCommandList& inoutCommandList, std::string_view aEventName, Texture& aShadowMap,
                                     const FrameBuffer& aFrameBuffer, const PipelineStateObject& aOverridePSO,
                                     PipeLineStages aOverrideStages, const void* aPointShadowBuffer,
                                     const std::vector<const RenderItemSnapshot*>& aRenderItems)
{
	inoutCommandList.BeginEvent(aEventName);
	UnbindShadowResources(inoutCommandList);
	inoutCommandList.ClearDepthStencil(aShadowMap);
	inoutCommandList.SetRenderTarget(nullptr, &aShadowMap);
	inoutCommandList.SetOverridePipelineState(aOverridePSO, aOverrideStages);
	UpdateAndSetConstantBuffer(inoutCommandList, ConstantBuffer::FrameBuffer, aFrameBuffer, 0, PipeLineStage_VertexShader);

	if (aPointShadowBuffer != nullptr)
	{
		UpdateAndSetConstantBufferInternal(inoutCommandList, ConstantBuffer::PointShadowBuffer, aPointShadowBuffer,
		                                   sizeof(PointShadowBufferData), 5, PipeLineStage_GeometryShader);
	}

	for (const RenderItemSnapshot* item : aRenderItems)
	{
		if (item != nullptr)
		{
			RenderMesh(inoutCommandList, *item, false);
		}
	}

	inoutCommandList.ClearOverridePipelineState();
	inoutCommandList.EndEvent();
}

float GraphicsEngine::GetShadowDepthBias(RenderLightType aType) const
{
	std::scoped_lock lock(myShadowTuningMutex);
	return GetShadowDepthBiasUnlocked(aType);
}

float GraphicsEngine::GetShadowDepthBiasUnlocked(RenderLightType aType) const
{
	float bias = ShadowConfig::DirectionalShaderBias + myDirectionalShadowBiasOffset;
	if (aType == RenderLightType::Spot)
	{
		bias = ShadowConfig::SpotShaderBias + mySpotShadowBiasOffset;
	}
	else if (aType == RenderLightType::Point)
	{
		bias = ShadowConfig::PointShaderBias + myPointShadowBiasOffset;
	}

	return std::clamp(bias, ShadowConfig::BiasMin, ShadowConfig::BiasMax);
}

void GraphicsEngine::AdjustShadowBias(RenderLightType aType, float aDelta)
{
	std::scoped_lock lock(myShadowTuningMutex);
	float* offset = &myDirectionalShadowBiasOffset;
	if (aType == RenderLightType::Spot)
	{
		offset = &mySpotShadowBiasOffset;
	}
	else if (aType == RenderLightType::Point)
	{
		offset = &myPointShadowBiasOffset;
	}

	*offset += aDelta;
	const float currentBias = GetShadowDepthBiasUnlocked(aType);
	if (currentBias <= ShadowConfig::BiasMin || currentBias >= ShadowConfig::BiasMax)
	{
		const float defaultBias = aType == RenderLightType::Spot    ? ShadowConfig::SpotShaderBias
		                          : aType == RenderLightType::Point ? ShadowConfig::PointShaderBias
		                                                            : ShadowConfig::DirectionalShaderBias;
		*offset = std::clamp(defaultBias + *offset, ShadowConfig::BiasMin, ShadowConfig::BiasMax) - defaultBias;
	}

	GELOG(Log, "Shadow {} bias: {:.6f}",
	      aType == RenderLightType::Directional ? "directional"
	      : aType == RenderLightType::Spot      ? "spot"
	                                            : "point",
	      GetShadowDepthBiasUnlocked(aType));
}

void GraphicsEngine::ResetShadowTuning()
{
	std::scoped_lock lock(myShadowTuningMutex);
	myDirectionalShadowBiasOffset = 0.0f;
	mySpotShadowBiasOffset = 0.0f;
	myPointShadowBiasOffset = 0.0f;
	GELOG(Log, "Shadow tuning reset.");
}

void GraphicsEngine::LogShadowTuning() const
{
	std::scoped_lock lock(myShadowTuningMutex);
	GELOG(
	    Log,
	    "Shadow tuning: cascades={}, splits={{ {:.1f}, {:.1f}, {:.1f}, {:.1f} }}, directionalBias={:.6f}, spotBias={:.6f}, pointBias={:.6f}, spotMaps={}, pointMaps={}",
	    ShadowConfig::DirectionalCascadeCount, ShadowConfig::CascadeSplits[0], ShadowConfig::CascadeSplits[1],
	    ShadowConfig::CascadeSplits[2], ShadowConfig::CascadeSplits[3], GetShadowDepthBiasUnlocked(RenderLightType::Directional),
	    GetShadowDepthBiasUnlocked(RenderLightType::Spot), GetShadowDepthBiasUnlocked(RenderLightType::Point), ShadowConfig::MaxSpotMaps,
	    ShadowConfig::MaxPointMaps);
}

bool GraphicsEngine::CreateConstantBuffer(ConstantBuffer aBufferId, std::string_view aName, size_t aBufferSize)
{
	return CreateConstantBufferInternal(aBufferId, aName, aBufferSize);
}

CU::Vector2u GraphicsEngine::GetClientSize() const
{
	return myRHI.GetClientSize();
}

bool GraphicsEngine::CreateCommandList(std::string_view aName, GraphicsCommandList& outCommandList) const
{
	return myRHI.CreateCommandList(aName, outCommandList);
}

void GraphicsEngine::ExecuteCommandList(const GraphicsCommandList& aCommandList) const
{
	myRHI.ExecuteCommandList(aCommandList);
}

// --- Deferred resources ---

bool GraphicsEngine::CreateGBufferResources()
{
	const CU::Vector2u clientSize = GetClientSize();
	const std::array<std::string_view, GBuffer::TargetCount> names = {"GBuffer_Albedo", "GBuffer_PixelNormal", "GBuffer_Surface",
	                                                                  "GBuffer_Emission", "GBuffer_WorldPosition"};
	const std::array<unsigned, GBuffer::TargetCount> formats = {
	    static_cast<unsigned>(DXGI_FORMAT_R8G8B8A8_UNORM), static_cast<unsigned>(DXGI_FORMAT_R16G16B16A16_SNORM),
	    static_cast<unsigned>(DXGI_FORMAT_R8G8B8A8_UNORM), static_cast<unsigned>(DXGI_FORMAT_R16G16B16A16_FLOAT),
	    static_cast<unsigned>(DXGI_FORMAT_R32G32B32A32_FLOAT)};
	for (size_t targetIndex = 0; targetIndex < names.size(); ++targetIndex)
	{
		if (!myRHI.CreateRenderTargetTexture(names[targetIndex], clientSize.x, clientSize.y, formats[targetIndex],
		                                     myGBuffer.GetTextures()[targetIndex]))
		{
			return false;
		}
	}

	return myRHI.CreateRenderTargetTexture("TangentNormal_Debug", clientSize.x, clientSize.y,
	                                       static_cast<unsigned>(DXGI_FORMAT_R16G16B16A16_SNORM), myTangentNormalDebugTexture) &&
	       myRHI.CreateRenderTargetTexture("Deferred_Lighting", clientSize.x, clientSize.y,
	                                       static_cast<unsigned>(DXGI_FORMAT_R32G32B32A32_FLOAT), myDeferredLightingTexture) &&
	       myRHI.CreateRenderTargetTexture("ScreenSpace_AO", clientSize.x, clientSize.y, static_cast<unsigned>(DXGI_FORMAT_R32_FLOAT),
	                                       myScreenSpaceAOTexture);
}

bool GraphicsEngine::CreateDeferredPipelineStates()
{
	Shader fullTextureVS;
	if (!myRHI.CompileShader(ShaderType::VertexShader, myShaderRoot / "Internal" / "FullTexture_VS.hlsl", nullptr, true, fullTextureVS))
	{
		return false;
	}

	auto createPipeline =
	    [this, &fullTextureVS](std::string_view aName, std::string_view aPixelShader, BlendMode aBlendMode, PipelineStateObject& outPSO)
	{
		const std::filesystem::path pixelShaderPath = myShaderRoot / "Internal" / aPixelShader;
		MaterialShaderIncludeHandler includeHandler(myShaderRoot, pixelShaderPath, {});
		Shader pixelShader;
		if (!myRHI.CompileShader(ShaderType::PixelShader, pixelShaderPath, &includeHandler, true, pixelShader))
		{
			return false;
		}
		PipelineStateDescription description;
		description.Name = aName;
		description.VertexShader.ByteCode = fullTextureVS.GetDataPtr();
		description.VertexShader.ByteCodeSize = fullTextureVS.GetDataSize();
		description.PixelShader.ByteCode = pixelShader.GetDataPtr();
		description.PixelShader.ByteCodeSize = pixelShader.GetDataSize();
		description.Topology = Topology::TriangleStrip;
		description.BlendMode = aBlendMode;
		return myRHI.CreatePipelineStateObject(description, outPSO);
	};

	return createPipeline("DeferredDirectionalPSO", "DeferredDirectional_PS.hlsl", BlendMode::Additive, myDeferredDirectionalPSO) &&
	       createPipeline("DeferredPointPSO", "DeferredPoint_PS.hlsl", BlendMode::Additive, myDeferredPointPSO) &&
	       createPipeline("DeferredSpotPSO", "DeferredSpot_PS.hlsl", BlendMode::Additive, myDeferredSpotPSO) &&
	       createPipeline("DeferredCompositePSO", "DeferredComposite_PS.hlsl", BlendMode::Opaque, myDeferredCompositePSO) &&
	       createPipeline("ScreenSpaceAOPSO", "ScreenSpaceAO_PS.hlsl", BlendMode::Opaque, myScreenSpaceAOPSO) &&
	       createPipeline("RenderPassDebugPSO", "RenderPassDebug_PS.hlsl", BlendMode::Opaque, myRenderPassDebugPSO);
}

// --- Image-based lighting resources ---

bool GraphicsEngine::CreatePBLResources()
{
	const std::filesystem::path environmentPath = myShaderRoot.parent_path() / "Textures" / "T_Shipyard.dds";
	if (!LoadTexture(environmentPath, myEnvironmentCubeTexture))
	{
		GELOG(Error, "Failed to load environment cube map '{}'.", environmentPath.string());
		return false;
	}

	return CreateBRDFLUT();
}

bool GraphicsEngine::CreateBRDFLUT()
{
	if (!myRHI.CreateRenderTargetTexture("BRDF_LUT", PBLConfig::BRDFLUTResolution, PBLConfig::BRDFLUTResolution,
	                                     static_cast<unsigned>(DXGI_FORMAT_R16G16_FLOAT), myBRDFLUTTexture))
	{
		return false;
	}

	Shader fullTextureVS;
	if (!myRHI.CompileShader(ShaderType::VertexShader, myShaderRoot / "Internal" / "FullTexture_VS.hlsl", nullptr, true, fullTextureVS))
	{
		return false;
	}

	Shader brdfLUTPS;
	if (!myRHI.CompileShader(ShaderType::PixelShader, myShaderRoot / "Internal" / "BRDF_LUT_PS.hlsl", nullptr, true, brdfLUTPS))
	{
		return false;
	}

	PipelineStateDescription psoDesc;
	psoDesc.Name = "BRDF_LUT_PSO";
	psoDesc.VertexShader.ByteCode = fullTextureVS.GetDataPtr();
	psoDesc.VertexShader.ByteCodeSize = fullTextureVS.GetDataSize();
	psoDesc.PixelShader.ByteCode = brdfLUTPS.GetDataPtr();
	psoDesc.PixelShader.ByteCodeSize = brdfLUTPS.GetDataSize();
	psoDesc.Topology = Topology::TriangleStrip;

	PipelineStateObject brdfLUTPSO;
	if (!myRHI.CreatePipelineStateObject(psoDesc, brdfLUTPSO))
	{
		return false;
	}

	GraphicsCommandList commandList;
	if (!CreateCommandList("BRDF LUT", commandList))
	{
		return false;
	}

	commandList.BeginEvent("Generate BRDF LUT");
	commandList.ClearRenderTarget(myBRDFLUTTexture);
	commandList.SetRenderTarget(&myBRDFLUTTexture, nullptr);
	commandList.SetPipelineState(&brdfLUTPSO);
	commandList.Draw(4);
	commandList.EndEvent();
	commandList.FinishCommandList();
	ExecuteCommandList(commandList);

	return true;
}

// --- Shadow resources ---

bool GraphicsEngine::CreateShadowResources()
{
	for (size_t cascadeIndex = 0; cascadeIndex < myDirectionalShadowMaps.size(); ++cascadeIndex)
	{
		if (!CreateShadowMap(std::format("DirectionalShadowCascade{}", cascadeIndex), ShadowConfig::MapResolution,
		                     ShadowConfig::MapResolution, myDirectionalShadowMaps[cascadeIndex]))
		{
			return false;
		}
	}

	for (size_t spotIndex = 0; spotIndex < mySpotShadowMaps.size(); ++spotIndex)
	{
		if (!CreateShadowMap(std::format("SpotShadow{}", spotIndex), ShadowConfig::MapResolution, ShadowConfig::MapResolution,
		                     mySpotShadowMaps[spotIndex]))
		{
			return false;
		}
	}

	for (size_t pointIndex = 0; pointIndex < myPointShadowMaps.size(); ++pointIndex)
	{
		if (!CreateShadowMap(std::format("PointShadow{}", pointIndex), ShadowConfig::MapResolution, ShadowConfig::MapResolution,
		                     myPointShadowMaps[pointIndex], true))
		{
			return false;
		}
	}

	return true;
}

bool GraphicsEngine::CreateShadowPipelineStates()
{
	RasterizerStateDescription shadowRasterizer;
	shadowRasterizer.CullMode = RasterizerCullMode::Front;
	shadowRasterizer.DepthBias = ShadowConfig::DirectionalRasterDepthBias;
	shadowRasterizer.SlopeScaledDepthBias = ShadowConfig::DirectionalRasterSlopeBias;

	PipelineStateDescription shadowPSODesc;
	shadowPSODesc.Name = "ShadowOverridePSO";
	shadowPSODesc.Topology = Topology::TriangleList;
	shadowPSODesc.RasterizerState = shadowRasterizer;
	if (!myRHI.CreatePipelineStateObject(shadowPSODesc, myShadowOverridePSO))
	{
		return false;
	}

	RasterizerStateDescription localShadowRasterizer;
	localShadowRasterizer.CullMode = RasterizerCullMode::None;
	localShadowRasterizer.DepthBias = ShadowConfig::LocalRasterDepthBias;
	localShadowRasterizer.SlopeScaledDepthBias = ShadowConfig::LocalRasterSlopeBias;

	PipelineStateDescription localShadowPSODesc;
	localShadowPSODesc.Name = "LocalShadowOverridePSO";
	localShadowPSODesc.Topology = Topology::TriangleList;
	localShadowPSODesc.RasterizerState = localShadowRasterizer;
	if (!myRHI.CreatePipelineStateObject(localShadowPSODesc, myLocalShadowOverridePSO))
	{
		return false;
	}

	Shader pointShadowGS;
	if (!myRHI.CompileShader(ShaderType::GeometryShader, myShaderRoot / "Internal" / "PointShadow_GS.hlsl", nullptr, true, pointShadowGS))
	{
		return false;
	}

	PipelineStateDescription pointShadowPSODesc = localShadowPSODesc;
	pointShadowPSODesc.Name = "PointShadowOverridePSO";
	pointShadowPSODesc.GeometryShader.ByteCode = pointShadowGS.GetDataPtr();
	pointShadowPSODesc.GeometryShader.ByteCodeSize = pointShadowGS.GetDataSize();
	return myRHI.CreatePipelineStateObject(pointShadowPSODesc, myPointShadowOverridePSO);
}

// --- Materials and textures ---

bool GraphicsEngine::CreateMaterial(const MaterialDescription& aDescription, Material& outMaterial) const
{
	Shader materialVS;
	Shader materialPS;
	Shader gbufferPS;

	if (aDescription.ShadingModel == ShadingModel::None)
	{
		GELOG(Error, "Material {} has invalid shading model!", aDescription.Name);
		return false;
	}
	if (aDescription.Domain == MaterialDomain::None)
	{
		GELOG(Error, "Material {} has invalid material domain!", aDescription.Name);
		return false;
	}
	if (aDescription.Name.empty())
	{
		GELOG(Error, "Material has no name!");
		return false;
	}

	{
		const std::filesystem::path& path = myMaterialDomainShaders.at(aDescription.Domain);
		MaterialShaderIncludeHandler handler(myShaderRoot / "Material", path, aDescription.MaterialShaderCode);
		if (!myRHI.CompileShader(ShaderType::VertexShader, path, &handler, true, materialVS))
		{
			return false;
		}
	}

	{
		const std::filesystem::path& path = myMaterialShadingModelShaders.at(aDescription.ShadingModel);
		MaterialShaderIncludeHandler handler(myShaderRoot / "Material", path, aDescription.MaterialShaderCode);
		if (!myRHI.CompileShader(ShaderType::PixelShader, path, &handler, true, materialPS))
		{
			return false;
		}
	}

	const std::filesystem::path gbufferPath = myShaderRoot / "Material" / "GBuffer_PS.hlsl";
	MaterialShaderIncludeHandler gbufferHandler(myShaderRoot / "Material", gbufferPath, aDescription.MaterialShaderCode);
	if (!myRHI.CompileShader(ShaderType::PixelShader, gbufferPath, &gbufferHandler, true, gbufferPS))
	{
		return false;
	}

	memset(outMaterial.myData, 0, Material::MATERIAL_BUFFER_SIZE);
	outMaterial.myParameters.clear();
	outMaterial.myParameterNameToIndex.clear();
	outMaterial.myTextureSlotNameToIndex.clear();
	for (std::shared_ptr<Texture>& texture : outMaterial.myTextures)
	{
		texture.reset();
	}

	RHIShaderReflectionInfo vsInfo, psInfo;
	RHIShaderReflector::Reflect(materialVS.GetDataPtr(), materialVS.GetDataSize(), vsInfo);
	RHIShaderReflector::Reflect(materialPS.GetDataPtr(), materialPS.GetDataSize(), psInfo);

	const RHIShaderReflectionInfo* materialBufferSource = nullptr;
	static std::string materialBufferName = "MaterialBuffer";
	if (vsInfo.ConstantBufferNameToIndex.contains(materialBufferName))
	{
		materialBufferSource = &vsInfo;
	}
	else if (psInfo.ConstantBufferNameToIndex.contains(materialBufferName))
	{
		materialBufferSource = &psInfo;
	}

	if (materialBufferSource)
	{
		const RHIShaderReflectionInfo::ConstantBufferInfo& info =
		    materialBufferSource->ConstantBuffers[materialBufferSource->ConstantBufferNameToIndex.at(materialBufferName)];

		for (size_t i = 0; i < info.Members.size(); ++i)
		{
			const auto& member = info.Members[i];
			MaterialParameterInfo param;
			const unsigned parameterIndex = static_cast<unsigned>(outMaterial.myParameters.size());
			param.Name = member.Name;
			param.Type = MaterialHelpers::HLSLTypeToMaterialParameterType(member.Type);
			param.Size = member.Size;
			param.Offset = member.Offset;
			param.Index = parameterIndex;

			memcpy_s(outMaterial.myData + param.Offset, param.Size, member.Default, param.Size);

			outMaterial.myParameterNameToIndex.emplace(param.Name, parameterIndex);
			outMaterial.myParameters.emplace_back(std::move(param));
		}
	}

	PipelineStateDescription matPSOdesc;
	matPSOdesc.Name = std::format("{}_MAT_PSO", aDescription.Name);
	matPSOdesc.VertexShader.ByteCode = materialVS.GetDataPtr();
	matPSOdesc.VertexShader.ByteCodeSize = materialVS.GetDataSize();
	matPSOdesc.PixelShader.ByteCode = materialPS.GetDataPtr();
	matPSOdesc.PixelShader.ByteCodeSize = materialPS.GetDataSize();
	matPSOdesc.InputLayoutElements = Vertex::Description;
	matPSOdesc.Topology = Topology::TriangleList;
	matPSOdesc.BlendMode = aDescription.BlendMode;

	PipelineStateObject matPSO;
	if (!myRHI.CreatePipelineStateObject(matPSOdesc, matPSO))
	{
		return false;
	}

	PipelineStateDescription gbufferPSOdesc = matPSOdesc;
	gbufferPSOdesc.Name = std::format("{}_GBUFFER_PSO", aDescription.Name);
	gbufferPSOdesc.PixelShader.ByteCode = gbufferPS.GetDataPtr();
	gbufferPSOdesc.PixelShader.ByteCodeSize = gbufferPS.GetDataSize();
	gbufferPSOdesc.BlendMode = BlendMode::Opaque;
	PipelineStateObject gbufferPSO;
	if (!myRHI.CreatePipelineStateObject(gbufferPSOdesc, gbufferPSO))
	{
		return false;
	}

	CreateMaterialTextureSlots(vsInfo, outMaterial);
	CreateMaterialTextureSlots(psInfo, outMaterial);

	auto loadTextureOrFallback =
	    [this](const std::filesystem::path& aTexturePath, const std::shared_ptr<Texture>& aFallback, std::string_view aTextureLabel)
	{
		if (!aTexturePath.empty())
		{
			std::shared_ptr<Texture> texture = std::make_shared<Texture>();
			if (LoadTexture(aTexturePath, *texture))
			{
				return texture;
			}

			GELOG(Warning, "Falling back to default {} texture because {} could not be loaded.", aTextureLabel, aTexturePath.string());
		}

		return aFallback;
	};

	outMaterial.SetTexture(Material::ALBEDO_TEXTURE_SLOT,
	                       loadTextureOrFallback(aDescription.AlbedoTexture, myDefaultAlbedoTexture, "albedo"));
	outMaterial.SetTexture(Material::NORMAL_TEXTURE_SLOT,
	                       loadTextureOrFallback(aDescription.NormalTexture, myDefaultNormalTexture, "normal"));
	outMaterial.SetTexture(Material::MATERIAL_TEXTURE_SLOT,
	                       loadTextureOrFallback(aDescription.MaterialTexture, myDefaultMaterialTexture, "material"));

	outMaterial.myPSO = matPSO;
	outMaterial.myGBufferPSO = gbufferPSO;
	outMaterial.myName = aDescription.Name;
	outMaterial.myDescription = aDescription;

	return true;
}

bool GraphicsEngine::CreateDefaultTextures()
{
	myDefaultAlbedoTexture = std::make_shared<Texture>();
	if (!myRHI.CreateColorTexture("Default_Albedo_White", std::array<uint8_t, 4>{255, 255, 255, 255}, *myDefaultAlbedoTexture))
	{
		GELOG(Error, "Failed to create default albedo texture.");
		return false;
	}

	myDefaultNormalTexture = std::make_shared<Texture>();
	if (!myRHI.CreateColorTexture("Default_Normal_Flat", std::array<uint8_t, 4>{128, 128, 255, 255}, *myDefaultNormalTexture))
	{
		GELOG(Error, "Failed to create default normal texture.");
		return false;
	}

	myDefaultMaterialTexture = std::make_shared<Texture>();
	if (!myRHI.CreateColorTexture("Default_Material_ORM", std::array<uint8_t, 4>{255, 128, 0, 255}, *myDefaultMaterialTexture))
	{
		GELOG(Error, "Failed to create default material texture.");
		return false;
	}

	return true;
}

bool GraphicsEngine::LoadTexture(const std::filesystem::path& aPath, Texture& outTexture) const
{
	if (!std::filesystem::exists(aPath))
	{
		GELOG(Warning, "Texture path {} does not exist!", aPath.string());
		return false;
	}

	std::ifstream file(aPath, std::ios::binary | std::ios::ate);
	if (!file)
	{
		GELOG(Error, "Failed to load texture {}! Could not open file!", aPath.string());
		return false;
	}

	const std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<uint8_t> fileData(size);
	file.read(reinterpret_cast<char*>(fileData.data()), size);
	file.close();

	return myRHI.CreateTexture(aPath.stem().string(), fileData.data(), fileData.size(), outTexture);
}

bool GraphicsEngine::CreateShadowMap(std::string_view aName, unsigned aWidth, unsigned aHeight, Texture& outShadowMap, bool aCubeMap) const
{
	return myRHI.CreateDepthStencil(aName, aWidth, aHeight, outShadowMap, aCubeMap);
}

// --- Constant buffers ---

bool GraphicsEngine::CreateConstantBufferInternal(ConstantBuffer aBufferId, std::string_view aName, size_t aBufferSize)
{
	if (myConstantBuffersFrozen)
	{
		GELOG(Error, "Constant-buffer registration is closed after initialization.");
		return false;
	}
	Buffer buffer;
	if (!myRHI.CreateConstantBuffer(aName, aBufferSize, buffer))
	{
		return false;
	}

	myConstantBuffers.emplace(aBufferId, std::move(buffer));
	return true;
}

bool GraphicsEngine::UpdateAndSetConstantBufferInternal(GraphicsCommandList& inoutCommandList, ConstantBuffer aBufferId, const void* aData,
                                                        size_t aDataSize, unsigned aSlot, PipeLineStages aStages)
{
	if (!myConstantBuffers.contains(aBufferId))
	{
		GELOG(Warning, "Requested constant buffer update failed because this buffer does not exist!");
		return false;
	}

	const Buffer& buffer = std::as_const(myConstantBuffers).at(aBufferId);
	if (!inoutCommandList.UpdateConstantBuffer(buffer, aData, aDataSize))
	{
		return false;
	}

	inoutCommandList.SetConstantBuffer(&buffer, aSlot, aStages);
	return true;
}

void GraphicsEngine::CreateMaterialTextureSlots(const RHIShaderReflectionInfo& aShaderInfo, Material& inoutMaterial) const
{
	for (const auto& shaderTextureSlot : aShaderInfo.Bindings)
	{
		if (shaderTextureSlot.Type != 2 || shaderTextureSlot.BindPoint >= Material::MAX_MATERIAL_TEXTURE_COUNT)
		{
			continue;
		}

		std::string lowerName = shaderTextureSlot.Name;
		std::ranges::transform(lowerName, lowerName.begin(), [](unsigned char aChar)
		{
			return static_cast<char>(std::tolower(aChar));
		});

		if (inoutMaterial.myTextureSlotNameToIndex.contains(lowerName) &&
		    inoutMaterial.myTextureSlotNameToIndex.at(lowerName) != shaderTextureSlot.BindPoint)
		{
			GELOG(Warning, "Found texture {} in multiple places when setting up material. Only the first instance will be used!",
			      shaderTextureSlot.Name);
			continue;
		}

		inoutMaterial.myTextureSlotNameToIndex.emplace(lowerName, shaderTextureSlot.BindPoint);
	}
}

GraphicsEngine::GraphicsEngine() = default;
GraphicsEngine::~GraphicsEngine() = default;

// --- Resource preparation ---

void GraphicsEngine::PrepareSnapshotRenderResources(const RenderSceneSnapshot& aSnapshot) const
{
	for (const RenderItemSnapshot& item : aSnapshot.ShadowCasters)
	{
		PrepareRenderItemResources(item);
	}
}

bool GraphicsEngine::PrepareRenderItemResources(const RenderItemSnapshot& aRenderItem) const
{
	const std::shared_ptr<Mesh>& mesh = aRenderItem.Mesh;
	if (mesh == nullptr)
	{
		return false;
	}

	if (!PrepareMeshForRendering(*mesh))
	{
		return false;
	}

	const std::vector<std::shared_ptr<MaterialInterface>>& materials = aRenderItem.Materials;
	for (const Mesh::Element& element : mesh->myElements)
	{
		const MaterialInterface* elementMaterial = &myDefaultMaterial;
		if (element.MaterialIndex < materials.size() && materials[element.MaterialIndex] != nullptr)
		{
			elementMaterial = materials[element.MaterialIndex].get();
		}

		if (elementMaterial->HasParameters() && elementMaterial->IsMaterialDataDirty())
		{
			elementMaterial->RefreshMaterialData();
		}
	}

	return true;
}

bool GraphicsEngine::EnsureShadowCommandListCount(size_t aCount)
{
	while (myShadowCommandLists.size() < aCount)
	{
		GraphicsCommandList commandList;
		if (!CreateCommandList(std::format("Shadow Worker {}", myShadowCommandLists.size()), commandList))
		{
			return false;
		}

		myShadowCommandLists.emplace_back(std::move(commandList));
	}

	return true;
}

void GraphicsEngine::StoreLastRenderStats(const RenderStats& aStats)
{
	std::scoped_lock lock(myRenderStatsMutex);
	myLastRenderStats = aStats;
}

GraphicsEngine::RenderStats GraphicsEngine::GetLastRenderStats() const
{
	std::scoped_lock lock(myRenderStatsMutex);
	return myLastRenderStats;
}

// --- Mesh submission ---

void GraphicsEngine::RenderMesh(GraphicsCommandList& inoutCommandList, const RenderItemSnapshot& aRenderItem, bool aAllowLazyPrepare,
                                RenderBlendFilter aBlendFilter, bool aUseGBufferPSO)
{
	const std::shared_ptr<Mesh>& mesh = aRenderItem.Mesh;
	const std::vector<std::shared_ptr<MaterialInterface>>& materials = aRenderItem.Materials;
	if (mesh == nullptr)
	{
		return;
	}

	if (aAllowLazyPrepare)
	{
		if (!PrepareMeshForRendering(*mesh))
		{
			return;
		}
	}
	else
	{
		ensure(mesh->myVertexBuffer.IsValid());
		ensure(mesh->myIndexBuffer.IsValid());
		if (!mesh->myVertexBuffer.IsValid() || !mesh->myIndexBuffer.IsValid())
		{
			return;
		}
	}

	inoutCommandList.SetVertexBuffer(&mesh->myVertexBuffer);
	inoutCommandList.SetIndexBuffer(&mesh->myIndexBuffer);

	ObjectBuffer ob;
	ob.World = aRenderItem.World;
	ob.WorldInvT = aRenderItem.World.GetInverseTranspose3x3();
	ob.HasSkinning = aRenderItem.HasSkinning ? 1u : 0u;
	UpdateAndSetConstantBuffer(inoutCommandList, ConstantBuffer::ObjectBuffer, ob, 1, PipeLineStage_VertexShader);

	if (aRenderItem.HasSkinning)
	{
		AnimationBuffer animationBuffer;
		animationBuffer.JointTransforms = aRenderItem.JointTransforms;
		UpdateAndSetConstantBuffer(inoutCommandList, ConstantBuffer::AnimationBuffer, animationBuffer, 2, PipeLineStage_VertexShader);
	}

	MaterialInterface* currentMaterial = nullptr;
	for (const Mesh::Element& element : mesh->GetElements())
	{
		MaterialInterface* elementMaterial = &myDefaultMaterial;
		if (element.MaterialIndex < materials.size() && materials[element.MaterialIndex] != nullptr)
		{
			elementMaterial = materials[element.MaterialIndex].get();
		}

		const BlendMode elementBlendMode = elementMaterial->GetBlendMode();
		if (aBlendFilter == RenderBlendFilter::OpaqueOnly && elementBlendMode != BlendMode::Opaque)
		{
			continue;
		}
		if (aBlendFilter == RenderBlendFilter::BlendedOnly && elementBlendMode == BlendMode::Opaque)
		{
			continue;
		}

		if (elementMaterial != currentMaterial)
		{
			currentMaterial = elementMaterial;
			// Only opaque elements write the GBuffer. Non-opaque materials retain
			// their normal Lit/Unlit forward shader; an Unlit Deferred path is not
			// supported or required.
			const bool useGBufferForElement = aUseGBufferPSO && elementBlendMode == BlendMode::Opaque;
			inoutCommandList.SetPipelineState(useGBufferForElement ? &currentMaterial->GetGBufferPSO() : &currentMaterial->GetPSO());

			if (currentMaterial->HasParameters())
			{
				if (currentMaterial->IsMaterialDataDirty())
				{
					ensure(aAllowLazyPrepare);
					if (!aAllowLazyPrepare)
					{
						return;
					}
					currentMaterial->RefreshMaterialData();
				}

				UpdateAndSetConstantBufferInternal(inoutCommandList, ConstantBuffer::MaterialBuffer,
				                                   currentMaterial->GetParameterDataBlock(), Material::MATERIAL_BUFFER_SIZE, 3,
				                                   PipeLineStage_VertexShader | PipeLineStage_PixelShader);
			}

			std::array<const Texture*, Material::MAX_MATERIAL_TEXTURE_COUNT> textures = {};
			for (size_t t = 0; t < Material::MAX_MATERIAL_TEXTURE_COUNT; ++t)
			{
				if (const std::shared_ptr<Texture>& texture = currentMaterial->GetTexture(static_cast<unsigned>(t)))
				{
					textures[t] = texture.get();
				}
			}
			inoutCommandList.SetShaderResources(textures.data(), textures.size(), 0,
			                                    PipeLineStage_VertexShader | PipeLineStage_PixelShader);
		}

		inoutCommandList.DrawIndexed(element.NumIndices, element.IndexOffset);
	}
}

bool GraphicsEngine::PrepareMeshForRendering(const Mesh& aMesh) const
{
	if (!aMesh.myVertexBuffer.IsValid())
	{
		if (!myRHI.CreateVertexBuffer(aMesh.myName, aMesh.myVertices, aMesh.myVertexBuffer))
		{
			GELOG(Warning, "Could not prepare mesh {}! Vertex buffer could not be created!", aMesh.myName);
			return false;
		}
	}

	if (!aMesh.myIndexBuffer.IsValid())
	{
		if (!myRHI.CreateIndexBuffer(aMesh.myName, aMesh.myIndices, aMesh.myIndexBuffer))
		{
			GELOG(Warning, "Could not prepare mesh {}! Index buffer could not be created!", aMesh.myName);
			return false;
		}
	}

	return true;
}
