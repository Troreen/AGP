#include "GraphicsEngine.pch.h"
#include "GraphicsEngine.h"

#include "ConstantBuffers/AnimationBuffer.h"
#include "ConstantBuffers/FrameBuffer.h"
#include "ConstantBuffers/LightBuffer.h"
#include "ConstantBuffers/ObjectBuffer.h"
#include "GameFramework/CameraComponent.h"
#include "GameFramework/LightComponent.h"
#include "GameFramework/MeshComponentBase.h"
#include "GameFramework/World.h"
#include "RHI/GraphicsCommandList.h"
#include "RHI/RHIShaderReflectionInfo.h"

#include "Materials/Material.h"
#include "Materials/MaterialShaderIncludeHandler.h"
#include "Materials/MaterialHelpers.h"
#include "Objects/Shader.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <limits>
#include <vector>

namespace
{
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
		constexpr std::array<float, DirectionalCascadeCount> CascadeSplits = { 150.0f, 600.0f, 1600.0f, 5000.0f };
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
		CU::Matrix4f ViewProjection;
		float DepthRange = 1.0f;
	};

	float GetRenderIntensity(const LightComponent& aLightComponent)
	{
		if (aLightComponent.GetLightType() == LightType::Directional)
		{
			return aLightComponent.GetIntensity();
		}

		return aLightComponent.GetIntensity() * 10000.0f;
	}

	CU::Matrix4f CreateNDCToTextureMatrix()
	{
		return {
			0.5f, 0.0f, 0.0f, 0.0f,
			0.0f, -0.5f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.5f, 0.5f, 0.0f, 1.0f
		};
	}

	CU::Vector3f GetLightUpVector(const CU::Vector3f& aDirection)
	{
		const float yAlignment = std::abs(aDirection.Dot(CU::Vector3f::UnitY));
		return yAlignment > 0.95f ? CU::Vector3f::UnitZ : CU::Vector3f::UnitY;
	}

	std::array<CU::Vector3f, 8> GetFrustumCorners(const CU::Camera3D& aCamera, float aNearPlane, float aFarPlane)
	{
		const CU::Vector3f position = aCamera.GetTransform().GetPosition();
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

		return {
			nearCenter - right * (nearWidth * 0.5f) + up * (nearHeight * 0.5f),
			nearCenter + right * (nearWidth * 0.5f) + up * (nearHeight * 0.5f),
			nearCenter + right * (nearWidth * 0.5f) - up * (nearHeight * 0.5f),
			nearCenter - right * (nearWidth * 0.5f) - up * (nearHeight * 0.5f),
			farCenter - right * (farWidth * 0.5f) + up * (farHeight * 0.5f),
			farCenter + right * (farWidth * 0.5f) + up * (farHeight * 0.5f),
			farCenter + right * (farWidth * 0.5f) - up * (farHeight * 0.5f),
			farCenter - right * (farWidth * 0.5f) - up * (farHeight * 0.5f)
		};
	}

	CascadeShadowData CreateCascadeShadowData(const CU::Camera3D& aCamera, const LightComponent& aLightComponent, float aNearPlane, float aFarPlane)
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

		const CU::Vector3f lightDirection = aLightComponent.GetWorldDirection().GetNormalized();
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

		const float xyPadding = (std::max)(
			ShadowConfig::DirectionalCascadeLightPaddingMin,
			radius * ShadowConfig::DirectionalCascadeLightPaddingScale);
		const float zPadding = (std::max)(
			ShadowConfig::DirectionalCascadeLightPaddingMin,
			radius * ShadowConfig::DirectionalCascadeDepthPaddingScale);

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

		const CU::Matrix4f projection = CU::Maths::CreateOrthographicLH(
			minX,
			maxX,
			minY,
			maxY,
			minZ,
			maxZ);

		CascadeShadowData data;
		data.ViewProjection = view * projection;
		data.DepthRange = (std::max)(maxZ - minZ, 1.0f);
		return data;
	}

	CU::Matrix4f CreateLightViewProjectionTexture(const CU::Matrix4f& aViewProjection)
	{
		return aViewProjection * CreateNDCToTextureMatrix();
	}

	CU::Matrix4f CreateSpotViewProjection(const LightComponent& aLightComponent)
	{
		const CU::Vector3f position = aLightComponent.GetWorldPosition();
		const CU::Vector3f direction = aLightComponent.GetWorldDirection().GetNormalized();
		const CU::Matrix4f view = CU::Maths::CreateLookAtLH(position, position + direction, GetLightUpVector(direction));
		const CU::Matrix4f projection = CU::Maths::CreatePerspectiveFovLH(
			aLightComponent.GetOuterCone() * 2.0f,
			1.0f,
			1.0f,
			aLightComponent.GetRadius());
		return view * projection;
	}

	PointShadowBufferData CreatePointShadowBuffer(const LightComponent& aLightComponent)
	{
		const CU::Vector3f position = aLightComponent.GetWorldPosition();
		const CU::Matrix4f projection = CU::Maths::CreatePerspectiveFovLH(
			CU::Maths::HalfPi<float>(),
			1.0f,
			1.0f,
			aLightComponent.GetRadius());

		const std::array<CU::Vector3f, 6> directions = {
			CU::Vector3f::UnitX,
			-CU::Vector3f::UnitX,
			CU::Vector3f::UnitY,
			-CU::Vector3f::UnitY,
			CU::Vector3f::UnitZ,
			-CU::Vector3f::UnitZ
		};

		const std::array<CU::Vector3f, 6> upVectors = {
			CU::Vector3f::UnitY,
			CU::Vector3f::UnitY,
			-CU::Vector3f::UnitZ,
			CU::Vector3f::UnitZ,
			CU::Vector3f::UnitY,
			CU::Vector3f::UnitY
		};

		PointShadowBufferData buffer;
		for (size_t face = 0; face < buffer.ViewProjection.size(); ++face)
		{
			buffer.ViewProjection[face] = CU::Maths::CreateLookAtLH(position, position + directions[face], upVectors[face]) * projection;
		}

		return buffer;
	}

	CU::Vector4f MakeShadowSettings(float aDepthBias)
	{
		return { aDepthBias, 0.0f, 0.0f, 0.0f };
	}

	LightBuffer::Light* AddLightToBuffer(const LightComponent& aLightComponent, LightBuffer& inoutLightBuffer, float aShadowDepthBias)
	{
		if (inoutLightBuffer.NumActiveLights >= LightBuffer::MaxLights)
		{
			return nullptr;
		}

		LightBuffer::Light& light = inoutLightBuffer.Lights[inoutLightBuffer.NumActiveLights++];
		light.Color = aLightComponent.GetColor();
		light.Intensity = GetRenderIntensity(aLightComponent);
		light.Position = aLightComponent.GetWorldPosition();
		light.Type = static_cast<unsigned>(aLightComponent.GetLightType());
		light.Direction = aLightComponent.GetWorldDirection();
		light.InnerCone = aLightComponent.GetInnerCone();
		light.OuterCone = aLightComponent.GetOuterCone();
		light.Radius = aLightComponent.GetRadius();
		light.ShadowMapIndex = 0;
		light.NumCascades = 0;
		light.CascadeSplits = CU::Vector4f::Zero;
		light.ShadowSettings = MakeShadowSettings(aShadowDepthBias);
		light.CascadeDepthBiases = CU::Vector4f::Zero;
		light.CascadeFilterWorldRadii = CU::Vector4f::Zero;
		return &light;
	}

}

GraphicsEngine& GraphicsEngine::Get()
{
	static GraphicsEngine myInstance;
	return myInstance;
}

bool GraphicsEngine::Initialize(HWND aWindowHandle, const std::filesystem::path& aShaderRoot)
{
	if (!std::filesystem::exists(aShaderRoot))
	{
		GELOG(Error, "Shader root dir is not a valid path! Provided root was {}.", aShaderRoot.string());
		return false;
	}

	myShaderRoot = aShaderRoot;

	if (!myRHI.Initialize(aWindowHandle, true, myBackBuffer, myDepthBuffer))
	{
		return false; // RHI logs this for us 
	}

	if (!CreateDefaultTextures())
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

	return true;
}

void GraphicsEngine::Render(GraphicsCommandList& inoutCommandList, const Actor& aCameraActor, const World& aWorld)
{
	CameraComponent* cameraComponent = aCameraActor.GetComponent<CameraComponent>();
	if (cameraComponent == nullptr)
	{
		GELOG(Warning, "Could not render world because camera actor '{}' has no CameraComponent.", aCameraActor.GetName());
		return;
	}

	cameraComponent->SyncCameraToOwner();
	const CU::Camera3D& camera = cameraComponent->GetCamera();

	const SceneRenderData sceneData = CollectRenderItemsAndLights(aWorld);
	UnbindShadowResources(inoutCommandList);

	LightBuffer lightBuffer;
	bool hasRenderedDirectionalShadow = false;
	unsigned spotShadowCount = 0;
	unsigned pointShadowCount = 0;

	for (const LightComponent* lightComponent : sceneData.LightComponents)
	{
		LightBuffer::Light* light = AddLightToBuffer(*lightComponent, lightBuffer, GetShadowDepthBias(lightComponent->GetLightType()));
		if (light == nullptr)
		{
			break;
		}

		if (lightComponent->GetLightType() == LightType::Directional && !hasRenderedDirectionalShadow)
		{
			RenderDirectionalShadows(inoutCommandList, camera, *lightComponent, *light, sceneData.RenderItems);
			hasRenderedDirectionalShadow = true;
		}
		else if (lightComponent->GetLightType() == LightType::Spot && spotShadowCount < MaxSpotShadowMaps)
		{
			RenderSpotShadows(inoutCommandList, *lightComponent, *light, spotShadowCount, sceneData.RenderItems);
			++spotShadowCount;
		}
		else if (lightComponent->GetLightType() == LightType::Point && pointShadowCount < MaxPointShadowMaps)
		{
			RenderPointShadows(inoutCommandList, *lightComponent, *light, pointShadowCount, sceneData.RenderItems);
			++pointShadowCount;
		}
	}

	inoutCommandList.ClearOverridePipelineState();

	inoutCommandList.ClearRenderTarget(myBackBuffer);
	inoutCommandList.ClearDepthStencil(myDepthBuffer);
	inoutCommandList.SetRenderTarget(&myBackBuffer, &myDepthBuffer);

	std::vector<Sampler*> samplerList(mySamplers.size());
	for (size_t s = 0; s < mySamplers.size(); ++s)
	{
		samplerList[s] = &mySamplers[s];
	}

	inoutCommandList.SetShaderSamplers(samplerList.data(), samplerList.size(), 0, PipeLineStage_VertexShader | PipeLineStage_PixelShader);
	BindPBLResources(inoutCommandList);
	BindShadowResources(inoutCommandList);

	FrameBuffer fb;
	fb.View = camera.GetViewMatrix();
	fb.Projection = camera.GetProjectionMatrix();
	const CU::Vector3f cameraPosition = camera.GetTransform().GetPosition();
	fb.CameraPosition = { cameraPosition.x, cameraPosition.y, cameraPosition.z, 1.0f };

	UpdateAndSetConstantBuffer(inoutCommandList, ConstantBuffer::FrameBuffer, fb, 0, PipeLineStage_VertexShader | PipeLineStage_PixelShader);
	UpdateAndSetConstantBuffer(inoutCommandList, ConstantBuffer::LightBuffer, lightBuffer, 4, PipeLineStage_PixelShader);

	for (const RenderItem& item : sceneData.RenderItems)
	{
		RenderMesh(inoutCommandList, *item.MeshComponent, item.World);
	}
}

void GraphicsEngine::Present() const
{
	myRHI.Present();
}

GraphicsEngine::SceneRenderData GraphicsEngine::CollectRenderItemsAndLights(const World& aWorld) const
{
	SceneRenderData data;
	for (const std::unique_ptr<Actor>& actor : aWorld.GetActors())
	{
		if (!actor || !actor->IsActive())
		{
			continue;
		}

		std::vector<LightComponent*> actorLights;
		actor->GetComponentsOfType(actorLights);
		for (const LightComponent* lightComponent : actorLights)
		{
			if (lightComponent != nullptr && lightComponent->IsEnabled())
			{
				data.LightComponents.emplace_back(lightComponent);
			}
		}

		std::vector<MeshComponentBase*> meshComponents;
		actor->GetComponentsOfType(meshComponents);
		for (const MeshComponentBase* meshComponent : meshComponents)
		{
			if (meshComponent != nullptr && meshComponent->IsEnabled() && meshComponent->HasMesh())
			{
				data.RenderItems.push_back({ meshComponent, actor->GetTransform().GetWorldMatrix() });
			}
		}
	}

	return data;
}

void GraphicsEngine::UnbindShadowResources(GraphicsCommandList& inoutCommandList) const
{
	std::array<const Texture*, ShadowConfig::DirectionalCascadeCount + ShadowConfig::MaxSpotMaps + ShadowConfig::MaxPointMaps> nullShadowResources = {};
	inoutCommandList.SetShaderResources(
		nullShadowResources.data(),
		nullShadowResources.size(),
		ShadowConfig::HighTextureSlotStart,
		PipeLineStage_PixelShader | PipeLineStage_GeometryShader);
}

void GraphicsEngine::BindPBLResources(GraphicsCommandList& inoutCommandList) const
{
	const std::array<const Texture*, 2> pblResources = { &myEnvironmentCubeTexture, &myBRDFLUTTexture };
	inoutCommandList.SetShaderResources(
		pblResources.data(),
		pblResources.size(),
		PBLConfig::EnvironmentCubeSlot,
		PipeLineStage_PixelShader);
}

void GraphicsEngine::BindShadowResources(GraphicsCommandList& inoutCommandList) const
{
	std::array<const Texture*, ShadowConfig::DirectionalCascadeCount + ShadowConfig::MaxSpotMaps + ShadowConfig::MaxPointMaps> shadowResources = {};
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

	inoutCommandList.SetShaderResources(
		shadowResources.data(),
		shadowResources.size(),
		ShadowConfig::HighTextureSlotStart,
		PipeLineStage_PixelShader);
}

void GraphicsEngine::RenderShadowMap(
	GraphicsCommandList& inoutCommandList,
	std::string_view aEventName,
	Texture& aShadowMap,
	const FrameBuffer& aFrameBuffer,
	const PipelineStateObject& aOverridePSO,
	PipeLineStages aOverrideStages,
	const void* aPointShadowBuffer,
	const std::vector<RenderItem>& aRenderItems)
{
	inoutCommandList.BeginEvent(aEventName);
	inoutCommandList.ClearDepthStencil(aShadowMap);
	inoutCommandList.SetRenderTarget(nullptr, &aShadowMap);
	inoutCommandList.SetOverridePipelineState(aOverridePSO, aOverrideStages);
	UpdateAndSetConstantBuffer(inoutCommandList, ConstantBuffer::FrameBuffer, aFrameBuffer, 0, PipeLineStage_VertexShader);

	if (aPointShadowBuffer != nullptr)
	{
		UpdateAndSetConstantBufferInternal(
			inoutCommandList,
			ConstantBuffer::PointShadowBuffer,
			aPointShadowBuffer,
			sizeof(PointShadowBufferData),
			5,
			PipeLineStage_GeometryShader);
	}

	for (const RenderItem& item : aRenderItems)
	{
		RenderMesh(inoutCommandList, *item.MeshComponent, item.World);
	}

	inoutCommandList.ClearOverridePipelineState();
	inoutCommandList.EndEvent();
}

void GraphicsEngine::RenderDirectionalShadows(
	GraphicsCommandList& inoutCommandList,
	const CU::Camera3D& aCamera,
	const LightComponent& aLightComponent,
	LightBuffer::Light& inoutLight,
	const std::vector<RenderItem>& aRenderItems)
{
	float cascadeNear = aCamera.GetNearPlane();
	std::array<CascadeShadowData, ShadowConfig::DirectionalCascadeCount> cascadeData = {};
	for (unsigned cascadeIndex = 0; cascadeIndex < ShadowConfig::DirectionalCascadeCount; ++cascadeIndex)
	{
		const float cascadeFar = ShadowConfig::CascadeSplits[cascadeIndex];
		const float cascadeLength = cascadeFar - cascadeNear;
		const float cascadePadding = (std::max)(
			ShadowConfig::DirectionalCascadeSplitPaddingMin,
			cascadeLength * ShadowConfig::DirectionalCascadeSplitPaddingScale);
		const float fitNear = (std::max)(aCamera.GetNearPlane(), cascadeNear - cascadePadding);
		const float fitFar = cascadeFar + cascadePadding;
		cascadeData[cascadeIndex] = CreateCascadeShadowData(aCamera, aLightComponent, fitNear, fitFar);
		const CU::Matrix4f lightViewProjection = cascadeData[cascadeIndex].ViewProjection;

		FrameBuffer shadowFrameBuffer;
		shadowFrameBuffer.View = CU::Matrix4f();
		shadowFrameBuffer.Projection = lightViewProjection;
		RenderShadowMap(
			inoutCommandList,
			std::format("Directional Shadow Cascade {}", cascadeIndex),
			myDirectionalShadowMaps[cascadeIndex],
			shadowFrameBuffer,
			myShadowOverridePSO,
			PipeLineStage_PixelShader | PipeLineStage_Rasterizer,
			nullptr,
			aRenderItems);

		inoutLight.LightViewProjTexture[cascadeIndex] = CreateLightViewProjectionTexture(lightViewProjection);
		cascadeNear = cascadeFar;
	}

	const float baseDirectionalBias = GetShadowDepthBias(LightType::Directional);
	const float referenceDepthRange = cascadeData[0].DepthRange;
	inoutLight.NumCascades = ShadowConfig::DirectionalCascadeCount;
	inoutLight.CascadeSplits = {
		ShadowConfig::CascadeSplits[0],
		ShadowConfig::CascadeSplits[1],
		ShadowConfig::CascadeSplits[2],
		ShadowConfig::CascadeSplits[3]
	};
	inoutLight.ShadowSettings = MakeShadowSettings(GetShadowDepthBias(LightType::Directional));
	inoutLight.CascadeDepthBiases = {
		baseDirectionalBias * referenceDepthRange / cascadeData[0].DepthRange,
		baseDirectionalBias * referenceDepthRange / cascadeData[1].DepthRange,
		baseDirectionalBias * referenceDepthRange / cascadeData[2].DepthRange,
		baseDirectionalBias * referenceDepthRange / cascadeData[3].DepthRange
	};
	inoutLight.CascadeFilterWorldRadii = {
		ShadowConfig::DirectionalFilterRadiusWorld,
		ShadowConfig::DirectionalFilterRadiusWorld,
		ShadowConfig::DirectionalFilterRadiusWorld,
		ShadowConfig::DirectionalFilterRadiusWorld
	};
}

void GraphicsEngine::RenderSpotShadows(
	GraphicsCommandList& inoutCommandList,
	const LightComponent& aLightComponent,
	LightBuffer::Light& inoutLight,
	unsigned aShadowIndex,
	const std::vector<RenderItem>& aRenderItems)
{
	const CU::Matrix4f lightViewProjection = CreateSpotViewProjection(aLightComponent);
	FrameBuffer shadowFrameBuffer;
	shadowFrameBuffer.View = CU::Matrix4f();
	shadowFrameBuffer.Projection = lightViewProjection;
	RenderShadowMap(
		inoutCommandList,
		std::format("Spot Shadow {}", aShadowIndex),
		mySpotShadowMaps[aShadowIndex],
		shadowFrameBuffer,
		myLocalShadowOverridePSO,
		PipeLineStage_PixelShader | PipeLineStage_Rasterizer,
		nullptr,
		aRenderItems);

	inoutLight.ShadowMapIndex = aShadowIndex;
	inoutLight.NumCascades = 1;
	inoutLight.LightViewProjTexture[0] = CreateLightViewProjectionTexture(lightViewProjection);
	inoutLight.ShadowSettings = MakeShadowSettings(GetShadowDepthBias(LightType::Spot));
}

void GraphicsEngine::RenderPointShadows(
	GraphicsCommandList& inoutCommandList,
	const LightComponent& aLightComponent,
	LightBuffer::Light& inoutLight,
	unsigned aShadowIndex,
	const std::vector<RenderItem>& aRenderItems)
{
	const PointShadowBufferData pointShadowBuffer = CreatePointShadowBuffer(aLightComponent);
	FrameBuffer shadowFrameBuffer;
	shadowFrameBuffer.View = CU::Matrix4f();
	shadowFrameBuffer.Projection = CU::Matrix4f();
	RenderShadowMap(
		inoutCommandList,
		std::format("Point Shadow {}", aShadowIndex),
		myPointShadowMaps[aShadowIndex],
		shadowFrameBuffer,
		myPointShadowOverridePSO,
		PipeLineStage_PixelShader | PipeLineStage_Rasterizer | PipeLineStage_GeometryShader,
		&pointShadowBuffer,
		aRenderItems);

	inoutLight.ShadowMapIndex = aShadowIndex;
	inoutLight.NumCascades = 1;
	inoutLight.ShadowSettings = MakeShadowSettings(GetShadowDepthBias(LightType::Point));
}

float GraphicsEngine::GetShadowDepthBias(LightType aType) const
{
	float bias = ShadowConfig::DirectionalShaderBias + myDirectionalShadowBiasOffset;
	if (aType == LightType::Spot)
	{
		bias = ShadowConfig::SpotShaderBias + mySpotShadowBiasOffset;
	}
	else if (aType == LightType::Point)
	{
		bias = ShadowConfig::PointShaderBias + myPointShadowBiasOffset;
	}

	return std::clamp(bias, ShadowConfig::BiasMin, ShadowConfig::BiasMax);
}

void GraphicsEngine::AdjustShadowBias(LightType aType, float aDelta)
{
	float* offset = &myDirectionalShadowBiasOffset;
	if (aType == LightType::Spot)
	{
		offset = &mySpotShadowBiasOffset;
	}
	else if (aType == LightType::Point)
	{
		offset = &myPointShadowBiasOffset;
	}

	*offset += aDelta;
	const float currentBias = GetShadowDepthBias(aType);
	if (currentBias <= ShadowConfig::BiasMin || currentBias >= ShadowConfig::BiasMax)
	{
		const float defaultBias =
			aType == LightType::Spot ? ShadowConfig::SpotShaderBias :
			aType == LightType::Point ? ShadowConfig::PointShaderBias :
			ShadowConfig::DirectionalShaderBias;
		*offset = std::clamp(defaultBias + *offset, ShadowConfig::BiasMin, ShadowConfig::BiasMax) - defaultBias;
	}

	GELOG(Log, "Shadow {} bias: {:.6f}", aType == LightType::Directional ? "directional" : aType == LightType::Spot ? "spot" : "point", GetShadowDepthBias(aType));
}

void GraphicsEngine::ResetShadowTuning()
{
	myDirectionalShadowBiasOffset = 0.0f;
	mySpotShadowBiasOffset = 0.0f;
	myPointShadowBiasOffset = 0.0f;
	GELOG(Log, "Shadow tuning reset.");
}

void GraphicsEngine::LogShadowTuning() const
{
	GELOG(Log, "Shadow tuning: cascades={}, splits={{ {:.1f}, {:.1f}, {:.1f}, {:.1f} }}, directionalBias={:.6f}, spotBias={:.6f}, pointBias={:.6f}, spotMaps={}, pointMaps={}",
		ShadowConfig::DirectionalCascadeCount,
		ShadowConfig::CascadeSplits[0],
		ShadowConfig::CascadeSplits[1],
		ShadowConfig::CascadeSplits[2],
		ShadowConfig::CascadeSplits[3],
		GetShadowDepthBias(LightType::Directional),
		GetShadowDepthBias(LightType::Spot),
		GetShadowDepthBias(LightType::Point),
		ShadowConfig::MaxSpotMaps,
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

bool GraphicsEngine::CreateCommandList(std::string_view aName, GraphicsCommandList &outCommandList) const
{
    return myRHI.CreateCommandList(aName, outCommandList);
}

void GraphicsEngine::ExecuteCommandList(const GraphicsCommandList &aCommandList) const
{
	myRHI.ExecuteCommandList(aCommandList);
}

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
	if (!myRHI.CreateRenderTargetTexture(
		"BRDF_LUT",
		PBLConfig::BRDFLUTResolution,
		PBLConfig::BRDFLUTResolution,
		static_cast<unsigned>(DXGI_FORMAT_R16G16_FLOAT),
		myBRDFLUTTexture))
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

bool GraphicsEngine::CreateShadowResources()
{
	for (size_t cascadeIndex = 0; cascadeIndex < myDirectionalShadowMaps.size(); ++cascadeIndex)
	{
		if (!CreateShadowMap(std::format("DirectionalShadowCascade{}", cascadeIndex), ShadowConfig::MapResolution, ShadowConfig::MapResolution, myDirectionalShadowMaps[cascadeIndex]))
		{
			return false;
		}
	}

	for (size_t spotIndex = 0; spotIndex < mySpotShadowMaps.size(); ++spotIndex)
	{
		if (!CreateShadowMap(std::format("SpotShadow{}", spotIndex), ShadowConfig::MapResolution, ShadowConfig::MapResolution, mySpotShadowMaps[spotIndex]))
		{
			return false;
		}
	}

	for (size_t pointIndex = 0; pointIndex < myPointShadowMaps.size(); ++pointIndex)
	{
		if (!CreateShadowMap(std::format("PointShadow{}", pointIndex), ShadowConfig::MapResolution, ShadowConfig::MapResolution, myPointShadowMaps[pointIndex], true))
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

bool GraphicsEngine::CreateMaterial(const MaterialDescription& aDescription, Material& outMaterial) const
{
	Shader materialVS;
	Shader materialPS;

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
	if(aDescription.BlendMode == BlendMode::None)
	{
		GELOG(Error, "Material {} has invalid blend mode!", aDescription.Name);
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
			return false;
	}

	{
		const std::filesystem::path& path = myMaterialShadingModelShaders.at(aDescription.ShadingModel);
		MaterialShaderIncludeHandler handler(myShaderRoot / "Material", path, aDescription.MaterialShaderCode);
		if (!myRHI.CompileShader(ShaderType::PixelShader, path, &handler, true, materialPS))
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
		const RHIShaderReflectionInfo::ConstantBufferInfo& info = materialBufferSource->ConstantBuffers[materialBufferSource->ConstantBufferNameToIndex.at(materialBufferName)];
		
		for (size_t i = 0; i < info.Members.size(); ++i)
		{
			const auto& member = info.Members[i];
			MaterialParameterInfo param;
			param.Name = member.Name;
			param.Type = MaterialHelpers::HLSLTypeToMaterialParameterType(member.Type);
			param.Size = member.Size;
			param.Offset = member.Offset;

			memcpy_s(outMaterial.myData + param.Offset, param.Size, member.Default, param.Size);

			outMaterial.myParameterNameToIndex.emplace(param.Name, static_cast<unsigned>(outMaterial.myParameters.size()));
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

	PipelineStateObject matPSO;
	if (!myRHI.CreatePipelineStateObject(matPSOdesc, matPSO))
	{
		return false;
	}

	CreateMaterialTextureSlots(vsInfo, outMaterial);
	CreateMaterialTextureSlots(psInfo, outMaterial);

	auto loadTextureOrFallback = [this](const std::filesystem::path& aTexturePath, const std::shared_ptr<Texture>& aFallback, std::string_view aTextureLabel)
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

	outMaterial.SetTexture(Material::ALBEDO_TEXTURE_SLOT, loadTextureOrFallback(aDescription.AlbedoTexture, myDefaultAlbedoTexture, "albedo"));
	outMaterial.SetTexture(Material::NORMAL_TEXTURE_SLOT, loadTextureOrFallback(aDescription.NormalTexture, myDefaultNormalTexture, "normal"));
	outMaterial.SetTexture(Material::MATERIAL_TEXTURE_SLOT, loadTextureOrFallback(aDescription.MaterialTexture, myDefaultMaterialTexture, "material"));

	outMaterial.myPSO = matPSO;
	outMaterial.myName = aDescription.Name;
	outMaterial.myDescription = aDescription;

	return true;
}

bool GraphicsEngine::CreateDefaultTextures()
{
	myDefaultAlbedoTexture = std::make_shared<Texture>();
	if (!myRHI.CreateColorTexture("Default_Albedo_White", std::array<uint8_t, 4>{ 255, 255, 255, 255 }, *myDefaultAlbedoTexture))
	{
		GELOG(Error, "Failed to create default albedo texture.");
		return false;
	}

	myDefaultNormalTexture = std::make_shared<Texture>();
	if (!myRHI.CreateColorTexture("Default_Normal_Flat", std::array<uint8_t, 4>{ 128, 128, 255, 255 }, *myDefaultNormalTexture))
	{
		GELOG(Error, "Failed to create default normal texture.");
		return false;
	}

	myDefaultMaterialTexture = std::make_shared<Texture>();
	if (!myRHI.CreateColorTexture("Default_Material_ORM", std::array<uint8_t, 4>{ 255, 128, 0, 255 }, *myDefaultMaterialTexture))
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

bool GraphicsEngine::CreateConstantBufferInternal(ConstantBuffer aBufferId, std::string_view aName, size_t aBufferSize)
{
	Buffer buffer;
	if (!myRHI.CreateConstantBuffer(aName, aBufferSize, buffer))
	{
		return false;
	}

	myConstantBuffers.emplace(aBufferId, std::move(buffer));
	return true;
}

bool GraphicsEngine::UpdateAndSetConstantBufferInternal(GraphicsCommandList& inoutCommandList, ConstantBuffer aBufferId, const void *aData, size_t aDataSize, unsigned aSlot, PipeLineStages aStages)
{
    if (!myConstantBuffers.contains(aBufferId))
	{
		GELOG(Warning, "Requested constant buffer update failed because this buffer does not exist!");
		return false;	
	}

	const Buffer& buffer = myConstantBuffers.at(aBufferId);
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
			continue;

		std::string lowerName = shaderTextureSlot.Name;
		std::ranges::transform(lowerName, lowerName.begin(), [](unsigned char aChar)
		{
			return static_cast<char>(std::tolower(aChar));
		});

		if (inoutMaterial.myTextureSlotNameToIndex.contains(lowerName) && inoutMaterial.myTextureSlotNameToIndex.at(lowerName) != shaderTextureSlot.BindPoint)
		{
			GELOG(Warning, "Found texture {} in multiple places when setting up material. Only the first instance will be used!", shaderTextureSlot.Name);
			continue;
		}

		inoutMaterial.myTextureSlotNameToIndex.emplace(lowerName, shaderTextureSlot.BindPoint);
	}

}

GraphicsEngine::GraphicsEngine() = default;
GraphicsEngine::~GraphicsEngine() = default;

void GraphicsEngine::RenderMesh(GraphicsCommandList& inoutCommandList, const MeshComponentBase& aMeshComponent, const CU::Matrix4f& aWorld)
{
	const std::shared_ptr<Mesh>& mesh = aMeshComponent.GetMesh();
	const std::vector<std::shared_ptr<MaterialInterface>>& materials = aMeshComponent.GetMaterialList();
	if (mesh == nullptr)
	{
		return;
	}

	if (!PrepareMeshForRendering(*mesh))
	{
		return;
	}

	inoutCommandList.SetVertexBuffer(&mesh->myVertexBuffer);
	inoutCommandList.SetIndexBuffer(&mesh->myIndexBuffer);

	ObjectBuffer ob;
	ob.World = aWorld;
	ob.WorldInvT = aWorld.GetInverseTranspose3x3();
	ob.HasSkinning = aMeshComponent.HasSkinning() ? 1u : 0u;
	UpdateAndSetConstantBuffer(inoutCommandList, ConstantBuffer::ObjectBuffer, ob, 1, PipeLineStage_VertexShader);

	if (aMeshComponent.HasSkinning())
	{
		AnimationBuffer animationBuffer;
		const std::array<CU::Matrix4f, 128>* jointTransforms = aMeshComponent.GetJointTransforms();
		if (jointTransforms != nullptr)
		{
			animationBuffer.JointTransforms = *jointTransforms;
			UpdateAndSetConstantBuffer(inoutCommandList, ConstantBuffer::AnimationBuffer, animationBuffer, 2, PipeLineStage_VertexShader);
		}
	}

	MaterialInterface* currentMaterial = nullptr;
	for (const Mesh::Element& element : mesh->myElements)
	{
		MaterialInterface* elementMaterial = &myDefaultMaterial;
		if (element.MaterialIndex < materials.size() && materials[element.MaterialIndex] != nullptr)
		{
			elementMaterial = materials[element.MaterialIndex].get();
		}

		if (elementMaterial != currentMaterial)
		{
			currentMaterial = elementMaterial;
			inoutCommandList.SetPipelineState(&currentMaterial->GetPSO());

			if (currentMaterial->HasParameters())
			{
				if (currentMaterial->IsMaterialDataDirty())
				{
					currentMaterial->RefreshMaterialData();
				}

				UpdateAndSetConstantBufferInternal(inoutCommandList, ConstantBuffer::MaterialBuffer, currentMaterial->GetParameterDataBlock(),
					Material::MATERIAL_BUFFER_SIZE, 3, PipeLineStage_VertexShader | PipeLineStage_PixelShader);
			}

			std::array<const Texture*, Material::MAX_MATERIAL_TEXTURE_COUNT> textures = {};
			for (size_t t = 0; t < Material::MAX_MATERIAL_TEXTURE_COUNT; ++t)
			{
				if (const std::shared_ptr<Texture>& texture = currentMaterial->GetTexture(static_cast<unsigned>(t)))
				{
					textures[t] = texture.get();
				}
			}
			inoutCommandList.SetShaderResources(textures.data(), textures.size(), 0, PipeLineStage_VertexShader | PipeLineStage_PixelShader);
		}

		inoutCommandList.DrawIndexed(element.NumIndices, element.IndexOffset);
	}
}

bool GraphicsEngine::PrepareMeshForRendering(const Mesh &aMesh) const
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
