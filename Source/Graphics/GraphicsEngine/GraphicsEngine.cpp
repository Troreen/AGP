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

#if defined(AGP_RENDERER_HOST_TEST_FAULTS)
#include "RendererHostFaultInjection.h"
#endif

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <future>
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

	struct BoundingSphere
	{
		CU::Vector3f Center = CU::Vector3f::Zero;
		float Radius = 0.0f;
		bool IsValid = false;
	};

	struct FrustumPlane
	{
		CU::Vector3f Normal = CU::Vector3f::Zero;
		float D = 0.0f;
		bool IsValid = false;
	};

	struct CameraFrustum
	{
		std::array<FrustumPlane, 6> Planes = {};
		bool IsValid = false;
	};

	using RenderItemPtrList = std::vector<const GraphicsEngine::RenderItemSnapshot*>;

	struct ShadowRenderJob
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

	FrustumPlane CreateFrustumPlane(const CU::Vector4f& aPlane)
	{
		FrustumPlane plane;
		plane.Normal = { aPlane.x, aPlane.y, aPlane.z };
		const float normalLength = plane.Normal.Length();
		if (normalLength <= 0.000001f)
		{
			return {};
		}

		const float invNormalLength = 1.0f / normalLength;
		plane.Normal *= invNormalLength;
		plane.D = aPlane.w * invNormalLength;
		plane.IsValid = true;
		return plane;
	}

	float GetSignedDistance(const FrustumPlane& aPlane, const CU::Vector3f& aPosition)
	{
		return aPlane.Normal.Dot(aPosition) + aPlane.D;
	}

	CameraFrustum CreateFrustumFromViewProjection(const CU::Matrix4f& aViewProjection)
	{
		const CU::Vector4f x = aViewProjection.GetColumn(1);
		const CU::Vector4f y = aViewProjection.GetColumn(2);
		const CU::Vector4f z = aViewProjection.GetColumn(3);
		const CU::Vector4f w = aViewProjection.GetColumn(4);

		CameraFrustum frustum;
		frustum.Planes[0] = CreateFrustumPlane(x + w);
		frustum.Planes[1] = CreateFrustumPlane(-x + w);
		frustum.Planes[2] = CreateFrustumPlane(y + w);
		frustum.Planes[3] = CreateFrustumPlane(-y + w);
		frustum.Planes[4] = CreateFrustumPlane(z);
		frustum.Planes[5] = CreateFrustumPlane(-z + w);
		frustum.IsValid = std::ranges::all_of(frustum.Planes, [](const FrustumPlane& aPlane)
		{
			return aPlane.IsValid;
		});
		return frustum;
	}

	CameraFrustum CreateCameraFrustum(const CU::Camera3D& aCamera)
	{
		return CreateFrustumFromViewProjection(aCamera.GetViewProjectionMatrix());
	}

	bool IntersectsFrustum(const CameraFrustum& aFrustum, const BoundingSphere& aSphere)
	{
		if (!aFrustum.IsValid)
		{
			return true;
		}

		if (!aSphere.IsValid)
		{
			return true;
		}

		for (const FrustumPlane& plane : aFrustum.Planes)
		{
			if (GetSignedDistance(plane, aSphere.Center) < -aSphere.Radius)
			{
				return false;
			}
		}

		return true;
	}

	bool IntersectsLightSpaceBounds(const CascadeShadowData& aCascade, const GraphicsEngine::RenderItemSnapshot& aRenderItem)
	{
		if (!aCascade.HasBounds || !aRenderItem.HasBounds)
		{
			return true;
		}

		const CU::Vector3f center = CU::Maths::TransformPoint(aRenderItem.BoundsCenter, aCascade.View);
		const float radius = aRenderItem.BoundsRadius;
		return center.x + radius >= aCascade.MinX
			&& center.x - radius <= aCascade.MaxX
			&& center.y + radius >= aCascade.MinY
			&& center.y - radius <= aCascade.MaxY
			&& center.z + radius >= aCascade.MinZ
			&& center.z - radius <= aCascade.MaxZ;
	}

	bool IntersectsPointLightRadius(const GraphicsEngine::LightSnapshot& aLight, const GraphicsEngine::RenderItemSnapshot& aRenderItem)
	{
		if (!aRenderItem.HasBounds || aLight.Radius <= 0.0f || !std::isfinite(aLight.Radius))
		{
			return true;
		}

		const float radius = aLight.Radius + aRenderItem.BoundsRadius;
		return (aRenderItem.BoundsCenter - aLight.Position).LengthSqr() <= radius * radius;
	}

	RenderItemPtrList CullCastersForCascade(const std::vector<GraphicsEngine::RenderItemSnapshot>& aRenderItems, const CascadeShadowData& aCascade)
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

	RenderItemPtrList CullCastersForFrustum(const std::vector<GraphicsEngine::RenderItemSnapshot>& aRenderItems, const CameraFrustum& aFrustum)
	{
		RenderItemPtrList visibleCasters;
		visibleCasters.reserve(aRenderItems.size());
		for (const GraphicsEngine::RenderItemSnapshot& item : aRenderItems)
		{
			if (!item.HasBounds || IntersectsFrustum(aFrustum, { item.BoundsCenter, item.BoundsRadius, item.HasBounds }))
			{
				visibleCasters.emplace_back(&item);
			}
		}
		return visibleCasters;
	}

	RenderItemPtrList CullCastersForPointLight(const std::vector<GraphicsEngine::RenderItemSnapshot>& aRenderItems, const GraphicsEngine::LightSnapshot& aLight)
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

	float GetMaxAxisScale(const CU::Matrix4f& aTransform)
	{
		const CU::Vector3f axisX(aTransform(1, 1), aTransform(1, 2), aTransform(1, 3));
		const CU::Vector3f axisY(aTransform(2, 1), aTransform(2, 2), aTransform(2, 3));
		const CU::Vector3f axisZ(aTransform(3, 1), aTransform(3, 2), aTransform(3, 3));
		return (std::max)({ axisX.Length(), axisY.Length(), axisZ.Length() });
	}

	BoundingSphere TransformBoundingSphere(const CU::Vector3f& aCenter, float aRadius, bool aIsValid, const CU::Matrix4f& aTransform)
	{
		if (!aIsValid)
		{
			return {};
		}

		BoundingSphere sphere;
		sphere.Center = CU::Maths::TransformPoint(aCenter, aTransform);
		sphere.Radius = aRadius * GetMaxAxisScale(aTransform);
		sphere.IsValid = sphere.Radius >= 0.0f && std::isfinite(sphere.Radius);
		return sphere;
	}

	float GetRenderIntensity(const GraphicsEngine::LightSnapshot& aLight)
	{
		if (aLight.Type == LightType::Directional)
		{
			return aLight.Intensity;
		}

		return aLight.Intensity * 10000.0f;
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

	CascadeShadowData CreateCascadeShadowData(const CU::Camera3D& aCamera, const GraphicsEngine::LightSnapshot& aLight, float aNearPlane, float aFarPlane)
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
		data.View = view;
		data.ViewProjection = view * projection;
		data.MinX = minX;
		data.MaxX = maxX;
		data.MinY = minY;
		data.MaxY = maxY;
		data.MinZ = minZ;
		data.MaxZ = maxZ;
		data.DepthRange = (std::max)(maxZ - minZ, 1.0f);
		data.HasBounds =
			std::isfinite(minX) && std::isfinite(maxX)
			&& std::isfinite(minY) && std::isfinite(maxY)
			&& std::isfinite(minZ) && std::isfinite(maxZ)
			&& minX < maxX
			&& minY < maxY
			&& minZ < maxZ;
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
		const CU::Matrix4f projection = CU::Maths::CreatePerspectiveFovLH(
			aLight.OuterCone * 2.0f,
			1.0f,
			1.0f,
			aLight.Radius);
		return view * projection;
	}

	PointShadowBufferData CreatePointShadowBuffer(const GraphicsEngine::LightSnapshot& aLight)
	{
		const CU::Vector3f position = aLight.Position;
		const CU::Matrix4f projection = CU::Maths::CreatePerspectiveFovLH(
			CU::Maths::HalfPi<float>(),
			1.0f,
			1.0f,
			aLight.Radius);

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

	GraphicsEngine::LightSnapshot CreateLightSnapshot(const LightComponent& aLightComponent)
	{
		GraphicsEngine::LightSnapshot light;
		light.Type = aLightComponent.GetLightType();
		light.Color = aLightComponent.GetColor();
		light.Intensity = aLightComponent.GetIntensity();
		light.Position = aLightComponent.GetWorldPosition();
		light.Direction = aLightComponent.GetWorldDirection();
		light.InnerCone = aLightComponent.GetInnerCone();
		light.OuterCone = aLightComponent.GetOuterCone();
		light.Radius = aLightComponent.GetRadius();
		return light;
	}

	bool IsRelevantLight(const CameraFrustum& aFrustum, const GraphicsEngine::LightSnapshot& aLight)
	{
		if (aLight.Type == LightType::Directional)
		{
			return true;
		}

		return IntersectsFrustum(aFrustum, { aLight.Position, aLight.Radius, true });
	}

}

void GraphicsEngine::RenderSceneSnapshot::Clear()
{
	HasCamera = false;
	ShadowCasters.clear();
	VisibleRenderItems.clear();
	RelevantLights.clear();
	Stats = {};
}

GraphicsEngine& GraphicsEngine::Get()
{
	static GraphicsEngine myInstance;
	return myInstance;
}

bool GraphicsEngine::Initialize(HWND aWindowHandle, const std::filesystem::path& aShaderRoot)
{
	return Initialize(aWindowHandle, aShaderRoot, aShaderRoot.parent_path() / "Textures" / "T_Shipyard.dds");
}

bool GraphicsEngine::Initialize(
	HWND aWindowHandle,
	const std::filesystem::path& aShaderRoot,
	const std::filesystem::path& aEnvironmentTexture)
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

	mySamplers.reserve(3);
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

	if (!CreatePBLResources(aEnvironmentTexture))
	{
		GELOG(Error, "Failed to create PBL resources.");
		return false;
	}

	return true;
}

void GraphicsEngine::Render(GraphicsCommandList& inoutCommandList, const Actor& aCameraActor, const World& aWorld)
{
	RenderSceneSnapshot snapshot;
	if (!BuildRenderSnapshot(aCameraActor, aWorld, snapshot))
	{
		return;
	}

	RenderSnapshot(inoutCommandList, snapshot);
}

bool GraphicsEngine::BuildRenderSnapshot(const Actor& aCameraActor, const World& aWorld, RenderSceneSnapshot& outSnapshot) const
{
	outSnapshot.Clear();

	CameraComponent* cameraComponent = aCameraActor.GetComponent<CameraComponent>();
	if (cameraComponent == nullptr)
	{
		GELOG(Warning, "Could not build render snapshot because camera actor '{}' has no CameraComponent.", aCameraActor.GetName());
		return false;
	}

	cameraComponent->SyncCameraToOwner();
	outSnapshot.Camera = cameraComponent->GetCamera();
	outSnapshot.HasCamera = true;

	const CameraFrustum cameraFrustum = CreateCameraFrustum(outSnapshot.Camera);
	const std::vector<std::unique_ptr<Actor>>& actors = aWorld.GetActors();
	outSnapshot.ShadowCasters.reserve(actors.size());
	outSnapshot.VisibleRenderItems.reserve(actors.size());
	outSnapshot.RelevantLights.reserve(actors.size());

	std::vector<LightComponent*> actorLights;
	std::vector<MeshComponentBase*> meshComponents;
	for (const std::unique_ptr<Actor>& actor : actors)
	{
		if (!actor || !actor->IsActive())
		{
			continue;
		}

		actorLights.clear();
		actor->GetComponentsOfType(actorLights);
		for (const LightComponent* lightComponent : actorLights)
		{
			if (lightComponent == nullptr || !lightComponent->IsEnabled())
			{
				continue;
			}

			++outSnapshot.Stats.TotalLights;
			const LightSnapshot light = CreateLightSnapshot(*lightComponent);
			if (IsRelevantLight(cameraFrustum, light))
			{
				outSnapshot.RelevantLights.emplace_back(light);
			}
		}

		meshComponents.clear();
		actor->GetComponentsOfType(meshComponents);
		for (const MeshComponentBase* meshComponent : meshComponents)
		{
			if (meshComponent == nullptr || !meshComponent->IsEnabled() || !meshComponent->HasMesh())
			{
				continue;
			}

			const std::shared_ptr<Mesh> mesh = meshComponent->GetMesh();
			if (mesh == nullptr)
			{
				continue;
			}

			RenderItemSnapshot renderItem;
			renderItem.Mesh = mesh;
			renderItem.Materials = meshComponent->GetMaterialList();
			renderItem.World = actor->GetTransform().GetWorldMatrix();
			renderItem.HasSkinning = meshComponent->HasSkinning();
			if (const std::array<CU::Matrix4f, 128>* jointTransforms = meshComponent->GetJointTransforms())
			{
				renderItem.JointTransforms = *jointTransforms;
			}

			const BoundingSphere worldBounds = TransformBoundingSphere(mesh->myLocalBoundsCenter, mesh->myLocalBoundsRadius, mesh->myHasLocalBounds, renderItem.World);
			renderItem.HasBounds = worldBounds.IsValid;
			renderItem.BoundsCenter = worldBounds.Center;
			renderItem.BoundsRadius = worldBounds.Radius;

			outSnapshot.ShadowCasters.emplace_back(renderItem);
			++outSnapshot.Stats.TotalRenderItems;
			if (!renderItem.HasBounds || IntersectsFrustum(cameraFrustum, worldBounds))
			{
				outSnapshot.VisibleRenderItems.emplace_back(std::move(renderItem));
			}
		}
	}

	outSnapshot.Stats.ShadowCasters = static_cast<uint32_t>(outSnapshot.ShadowCasters.size());
	outSnapshot.Stats.VisibleRenderItems = static_cast<uint32_t>(outSnapshot.VisibleRenderItems.size());
	outSnapshot.Stats.RelevantLights = static_cast<uint32_t>(outSnapshot.RelevantLights.size());
	return true;
}

bool GraphicsEngine::RenderSnapshot(GraphicsCommandList& inoutCommandList, const RenderSceneSnapshot& aSnapshot)
{
	if (!aSnapshot.HasCamera)
	{
		return false;
	}

	if (!PrepareSnapshotRenderResources(aSnapshot))
	{
		return false;
	}
	UnbindShadowResources(inoutCommandList);

	RenderStats frameStats = aSnapshot.Stats;
	LightBuffer lightBuffer;
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

		if (lightSnapshot.Type == LightType::Directional && !hasRenderedDirectionalShadow)
		{
			float cascadeNear = aSnapshot.Camera.GetNearPlane();
			std::array<CascadeShadowData, ShadowConfig::DirectionalCascadeCount> cascadeData = {};
			for (unsigned cascadeIndex = 0; cascadeIndex < ShadowConfig::DirectionalCascadeCount; ++cascadeIndex)
			{
				const float cascadeFar = ShadowConfig::CascadeSplits[cascadeIndex];
				const float cascadeLength = cascadeFar - cascadeNear;
				const float cascadePadding = (std::max)(
					ShadowConfig::DirectionalCascadeSplitPaddingMin,
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

			const float baseDirectionalBias = GetShadowDepthBias(LightType::Directional);
			const float referenceDepthRange = cascadeData[0].DepthRange;
			light->NumCascades = ShadowConfig::DirectionalCascadeCount;
			light->CascadeSplits = {
				ShadowConfig::CascadeSplits[0],
				ShadowConfig::CascadeSplits[1],
				ShadowConfig::CascadeSplits[2],
				ShadowConfig::CascadeSplits[3]
			};
			light->ShadowSettings = MakeShadowSettings(GetShadowDepthBias(LightType::Directional));
			light->CascadeDepthBiases = {
				baseDirectionalBias * referenceDepthRange / cascadeData[0].DepthRange,
				baseDirectionalBias * referenceDepthRange / cascadeData[1].DepthRange,
				baseDirectionalBias * referenceDepthRange / cascadeData[2].DepthRange,
				baseDirectionalBias * referenceDepthRange / cascadeData[3].DepthRange
			};
			light->CascadeFilterWorldRadii = {
				ShadowConfig::DirectionalFilterRadiusWorld,
				ShadowConfig::DirectionalFilterRadiusWorld,
				ShadowConfig::DirectionalFilterRadiusWorld,
				ShadowConfig::DirectionalFilterRadiusWorld
			};
			hasRenderedDirectionalShadow = true;
		}
		else if (lightSnapshot.Type == LightType::Spot && spotShadowCount < MaxSpotShadowMaps)
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
			light->ShadowSettings = MakeShadowSettings(GetShadowDepthBias(LightType::Spot));
			++spotShadowCount;
		}
		else if (lightSnapshot.Type == LightType::Point && pointShadowCount < MaxPointShadowMaps)
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
			light->ShadowSettings = MakeShadowSettings(GetShadowDepthBias(LightType::Point));
			++pointShadowCount;
		}
	}

	auto recordShadowJob = [this](GraphicsCommandList& inoutShadowCommandList, const ShadowRenderJob& aJob, bool aFinishCommandList)
	{
		ensure(aJob.ShadowMap != nullptr);
		ensure(aJob.OverridePSO != nullptr);
		RenderShadowMap(
			inoutShadowCommandList,
			aJob.EventName,
			*aJob.ShadowMap,
			aJob.FrameBufferData,
			*aJob.OverridePSO,
			aJob.OverrideStages,
			aJob.HasPointShadowBuffer ? &aJob.PointShadowBuffer : nullptr,
			aJob.RenderItems);

		if (aFinishCommandList)
		{
			inoutShadowCommandList.FinishCommandList();
		}
	};

	if (!shadowJobs.empty())
	{
		if (EnsureShadowCommandListCount(shadowJobs.size()))
		{
			std::vector<std::future<void>> shadowFutures;
			shadowFutures.reserve(shadowJobs.size());
			for (size_t jobIndex = 0; jobIndex < shadowJobs.size(); ++jobIndex)
			{
				GraphicsCommandList& commandList = myShadowCommandLists[jobIndex];
				commandList.ResetCommandList();
				shadowFutures.emplace_back(std::async(std::launch::async, [recordShadowJob, &commandList, &job = shadowJobs[jobIndex]]
				{
					recordShadowJob(commandList, job, true);
				}));
			}

			for (std::future<void>& future : shadowFutures)
			{
				future.get();
			}

			for (size_t jobIndex = 0; jobIndex < shadowJobs.size(); ++jobIndex)
			{
				ExecuteCommandList(myShadowCommandLists[jobIndex]);
			}

			frameStats.ShadowCommandListsRecorded = static_cast<uint32_t>(shadowJobs.size());
			frameStats.ShadowCommandListsExecuted = static_cast<uint32_t>(shadowJobs.size());
		}
		else
		{
			for (const ShadowRenderJob& job : shadowJobs)
			{
				recordShadowJob(inoutCommandList, job, false);
			}
		}
	}

	inoutCommandList.ClearOverridePipelineState();

	inoutCommandList.ClearRenderTarget(myBackBuffer);
	inoutCommandList.ClearDepthStencil(myDepthBuffer);
	inoutCommandList.SetRenderTarget(&myBackBuffer, &myDepthBuffer);

	inoutCommandList.SetShaderSamplers(mySamplerBindings.data(), mySamplerBindings.size(), 0, PipeLineStage_VertexShader | PipeLineStage_PixelShader);
	BindPBLResources(inoutCommandList);
	BindShadowResources(inoutCommandList);

	FrameBuffer fb;
	fb.View = aSnapshot.Camera.GetViewMatrix();
	fb.Projection = aSnapshot.Camera.GetProjectionMatrix();
	const CU::Vector3f cameraPosition = aSnapshot.Camera.GetTransform().GetPosition();
	fb.CameraPosition = { cameraPosition.x, cameraPosition.y, cameraPosition.z, 1.0f };

	UpdateAndSetConstantBuffer(inoutCommandList, ConstantBuffer::FrameBuffer, fb, 0, PipeLineStage_VertexShader | PipeLineStage_PixelShader);
	UpdateAndSetConstantBuffer(inoutCommandList, ConstantBuffer::LightBuffer, lightBuffer, 4, PipeLineStage_PixelShader);

	for (const RenderItemSnapshot& item : aSnapshot.VisibleRenderItems)
	{
		RenderMesh(inoutCommandList, item, false);
	}

	StoreLastRenderStats(frameStats);
	return true;
}

bool GraphicsEngine::Present() const
{
	return myRHI.Present();
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
		UpdateAndSetConstantBufferInternal(
			inoutCommandList,
			ConstantBuffer::PointShadowBuffer,
			aPointShadowBuffer,
			sizeof(PointShadowBufferData),
			5,
			PipeLineStage_GeometryShader);
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

void GraphicsEngine::RenderDirectionalShadows(
	GraphicsCommandList& inoutCommandList,
	const CU::Camera3D& aCamera,
	const LightSnapshot& aLight,
	LightBuffer::Light& inoutLight,
	const std::vector<RenderItemSnapshot>& aRenderItems)
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
		cascadeData[cascadeIndex] = CreateCascadeShadowData(aCamera, aLight, fitNear, fitFar);
		const CU::Matrix4f lightViewProjection = cascadeData[cascadeIndex].ViewProjection;

		FrameBuffer shadowFrameBuffer;
		shadowFrameBuffer.View = CU::Matrix4f();
		shadowFrameBuffer.Projection = lightViewProjection;
		RenderItemPtrList renderItems;
		renderItems.reserve(aRenderItems.size());
		for (const RenderItemSnapshot& item : aRenderItems)
		{
			renderItems.emplace_back(&item);
		}
		RenderShadowMap(
			inoutCommandList,
			std::format("Directional Shadow Cascade {}", cascadeIndex),
			myDirectionalShadowMaps[cascadeIndex],
			shadowFrameBuffer,
			myShadowOverridePSO,
			PipeLineStage_PixelShader | PipeLineStage_Rasterizer,
			nullptr,
			renderItems);

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
	const LightSnapshot& aLight,
	LightBuffer::Light& inoutLight,
	unsigned aShadowIndex,
	const std::vector<RenderItemSnapshot>& aRenderItems)
{
	const CU::Matrix4f lightViewProjection = CreateSpotViewProjection(aLight);
	FrameBuffer shadowFrameBuffer;
	shadowFrameBuffer.View = CU::Matrix4f();
	shadowFrameBuffer.Projection = lightViewProjection;
	RenderItemPtrList renderItems;
	renderItems.reserve(aRenderItems.size());
	for (const RenderItemSnapshot& item : aRenderItems)
	{
		renderItems.emplace_back(&item);
	}
	RenderShadowMap(
		inoutCommandList,
		std::format("Spot Shadow {}", aShadowIndex),
		mySpotShadowMaps[aShadowIndex],
		shadowFrameBuffer,
		myLocalShadowOverridePSO,
		PipeLineStage_PixelShader | PipeLineStage_Rasterizer,
		nullptr,
		renderItems);

	inoutLight.ShadowMapIndex = aShadowIndex;
	inoutLight.NumCascades = 1;
	inoutLight.LightViewProjTexture[0] = CreateLightViewProjectionTexture(lightViewProjection);
	inoutLight.ShadowSettings = MakeShadowSettings(GetShadowDepthBias(LightType::Spot));
}

void GraphicsEngine::RenderPointShadows(
	GraphicsCommandList& inoutCommandList,
	const LightSnapshot& aLight,
	LightBuffer::Light& inoutLight,
	unsigned aShadowIndex,
	const std::vector<RenderItemSnapshot>& aRenderItems)
{
	const PointShadowBufferData pointShadowBuffer = CreatePointShadowBuffer(aLight);
	FrameBuffer shadowFrameBuffer;
	shadowFrameBuffer.View = CU::Matrix4f();
	shadowFrameBuffer.Projection = CU::Matrix4f();
	RenderItemPtrList renderItems;
	renderItems.reserve(aRenderItems.size());
	for (const RenderItemSnapshot& item : aRenderItems)
	{
		renderItems.emplace_back(&item);
	}
	RenderShadowMap(
		inoutCommandList,
		std::format("Point Shadow {}", aShadowIndex),
		myPointShadowMaps[aShadowIndex],
		shadowFrameBuffer,
		myPointShadowOverridePSO,
		PipeLineStage_PixelShader | PipeLineStage_Rasterizer | PipeLineStage_GeometryShader,
		&pointShadowBuffer,
		renderItems);

	inoutLight.ShadowMapIndex = aShadowIndex;
	inoutLight.NumCascades = 1;
	inoutLight.ShadowSettings = MakeShadowSettings(GetShadowDepthBias(LightType::Point));
}

float GraphicsEngine::GetShadowDepthBias(LightType aType) const
{
	std::scoped_lock lock(myShadowTuningMutex);
	return GetShadowDepthBiasUnlocked(aType);
}

float GraphicsEngine::GetShadowDepthBiasUnlocked(LightType aType) const
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
	std::scoped_lock lock(myShadowTuningMutex);
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
	const float currentBias = GetShadowDepthBiasUnlocked(aType);
	if (currentBias <= ShadowConfig::BiasMin || currentBias >= ShadowConfig::BiasMax)
	{
		const float defaultBias =
			aType == LightType::Spot ? ShadowConfig::SpotShaderBias :
			aType == LightType::Point ? ShadowConfig::PointShaderBias :
			ShadowConfig::DirectionalShaderBias;
		*offset = std::clamp(defaultBias + *offset, ShadowConfig::BiasMin, ShadowConfig::BiasMax) - defaultBias;
	}

	GELOG(Log, "Shadow {} bias: {:.6f}", aType == LightType::Directional ? "directional" : aType == LightType::Spot ? "spot" : "point", GetShadowDepthBiasUnlocked(aType));
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
	GELOG(Log, "Shadow tuning: cascades={}, splits={{ {:.1f}, {:.1f}, {:.1f}, {:.1f} }}, directionalBias={:.6f}, spotBias={:.6f}, pointBias={:.6f}, spotMaps={}, pointMaps={}",
		ShadowConfig::DirectionalCascadeCount,
		ShadowConfig::CascadeSplits[0],
		ShadowConfig::CascadeSplits[1],
		ShadowConfig::CascadeSplits[2],
		ShadowConfig::CascadeSplits[3],
		GetShadowDepthBiasUnlocked(LightType::Directional),
		GetShadowDepthBiasUnlocked(LightType::Spot),
		GetShadowDepthBiasUnlocked(LightType::Point),
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

bool GraphicsEngine::BeginBackBufferFrame(const std::array<float, 4>& aClearColor) const
{
	return myRHI.BeginBackBufferFrame(myBackBuffer, myDepthBuffer, aClearColor);
}

RenderHardwareInterface::ResizeBackBufferResult GraphicsEngine::ResizeBackBuffer(unsigned aWidth, unsigned aHeight)
{
	return myRHI.ResizeBackBuffer(aWidth, aHeight, myBackBuffer, myDepthBuffer);
}

ID3D11Device* GraphicsEngine::GetNativeDevice() const
{
	return myRHI.GetNativeDevice();
}

ID3D11DeviceContext* GraphicsEngine::GetNativeImmediateContext() const
{
	return myRHI.GetNativeImmediateContext();
}

bool GraphicsEngine::CreatePBLResources(const std::filesystem::path& aEnvironmentTexture)
{
	if (!LoadTexture(aEnvironmentTexture, myEnvironmentCubeTexture))
	{
		GELOG(Error, "Failed to load environment cube map '{}'.", aEnvironmentTexture.string());
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
	return CreateMaterialInternal(aDescription, outMaterial, false) == MaterialCreationResult::Completed;
}

GraphicsEngine::MaterialCreationResult GraphicsEngine::CreateMaterialWithExactTextures(
	const MaterialDescription& aDescription,
	Material& outMaterial) const
{
	return CreateMaterialInternal(aDescription, outMaterial, true);
}

GraphicsEngine::MaterialCreationResult GraphicsEngine::CreateMaterialInternal(
	const MaterialDescription& aDescription,
	Material& outMaterial,
	bool aRequireExactTextures) const
{
	Shader materialVS;
	Shader materialPS;

	if (aDescription.ShadingModel == ShadingModel::None)
	{
		GELOG(Error, "Material {} has invalid shading model!", aDescription.Name);
		return MaterialCreationResult::DefinitionOrShaderFailed;
	}
	if (aDescription.Domain == MaterialDomain::None)
	{
		GELOG(Error, "Material {} has invalid material domain!", aDescription.Name);
		return MaterialCreationResult::DefinitionOrShaderFailed;
	}
	if (aDescription.BlendMode == BlendMode::None)
	{
		GELOG(Error, "Material {} has invalid blend mode!", aDescription.Name);
		return MaterialCreationResult::DefinitionOrShaderFailed;
	}
	if (aDescription.Name.empty())
	{
		GELOG(Error, "Material has no name!");
		return MaterialCreationResult::DefinitionOrShaderFailed;
	}

	{
		const std::filesystem::path& path = myMaterialDomainShaders.at(aDescription.Domain);
		MaterialShaderIncludeHandler handler(myShaderRoot / "Material", path, aDescription.MaterialShaderCode);
		if (!myRHI.CompileShader(ShaderType::VertexShader, path, &handler, true, materialVS))
		{
			return MaterialCreationResult::DefinitionOrShaderFailed;
		}
	}

	{
		const std::filesystem::path& path = myMaterialShadingModelShaders.at(aDescription.ShadingModel);
		MaterialShaderIncludeHandler handler(myShaderRoot / "Material", path, aDescription.MaterialShaderCode);
		if (!myRHI.CompileShader(ShaderType::PixelShader, path, &handler, true, materialPS))
		{
			return MaterialCreationResult::DefinitionOrShaderFailed;
		}
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
		return MaterialCreationResult::DefinitionOrShaderFailed;
	}

	CreateMaterialTextureSlots(vsInfo, outMaterial);
	CreateMaterialTextureSlots(psInfo, outMaterial);

	auto loadTextureOrFallback = [this, aRequireExactTextures](
		const std::filesystem::path& aTexturePath,
		const std::shared_ptr<Texture>& aFallback,
		std::string_view aTextureLabel,
		MaterialCreationResult aFailure,
		std::shared_ptr<Texture>& outTexture)
	{
		if (!aTexturePath.empty())
		{
			std::shared_ptr<Texture> texture = std::make_shared<Texture>();
			if (LoadTexture(aTexturePath, *texture))
			{
				outTexture = std::move(texture);
				return MaterialCreationResult::Completed;
			}

			if (aRequireExactTextures)
			{
				return aFailure;
			}
			GELOG(Warning, "Falling back to default {} texture because {} could not be loaded.", aTextureLabel, aTexturePath.string());
		}
		else if (aRequireExactTextures)
		{
			return aFailure;
		}

		outTexture = aFallback;
		return MaterialCreationResult::Completed;
	};

	std::shared_ptr<Texture> albedoTexture;
	std::shared_ptr<Texture> normalTexture;
	std::shared_ptr<Texture> materialTexture;
	MaterialCreationResult textureResult = loadTextureOrFallback(
		aDescription.AlbedoTexture, myDefaultAlbedoTexture, "albedo",
		MaterialCreationResult::AlbedoTextureFailed, albedoTexture);
	if (textureResult != MaterialCreationResult::Completed)
	{
		return textureResult;
	}
	textureResult = loadTextureOrFallback(
		aDescription.NormalTexture, myDefaultNormalTexture, "normal",
		MaterialCreationResult::NormalTextureFailed, normalTexture);
	if (textureResult != MaterialCreationResult::Completed)
	{
		return textureResult;
	}
	textureResult = loadTextureOrFallback(
		aDescription.MaterialTexture, myDefaultMaterialTexture, "material",
		MaterialCreationResult::MaterialTextureFailed, materialTexture);
	if (textureResult != MaterialCreationResult::Completed)
	{
		return textureResult;
	}

	outMaterial.SetTexture(Material::ALBEDO_TEXTURE_SLOT, albedoTexture);
	outMaterial.SetTexture(Material::NORMAL_TEXTURE_SLOT, normalTexture);
	outMaterial.SetTexture(Material::MATERIAL_TEXTURE_SLOT, materialTexture);

	outMaterial.myPSO = matPSO;
	outMaterial.myName = aDescription.Name;
	outMaterial.myDescription = aDescription;

	return MaterialCreationResult::Completed;
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

bool GraphicsEngine::PrepareSnapshotRenderResources(const RenderSceneSnapshot& aSnapshot) const
{
	for (const RenderItemSnapshot& item : aSnapshot.ShadowCasters)
	{
		if (!PrepareRenderItemResources(item))
		{
			return false;
		}
	}

	for (const RenderItemSnapshot& item : aSnapshot.VisibleRenderItems)
	{
		if (!PrepareRenderItemResources(item))
		{
			return false;
		}
	}

	return true;
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

void GraphicsEngine::RenderMesh(GraphicsCommandList& inoutCommandList, const RenderItemSnapshot& aRenderItem, bool aAllowLazyPrepare)
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
					ensure(aAllowLazyPrepare);
					if (!aAllowLazyPrepare)
					{
						return;
					}
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
#if defined(AGP_RENDERER_HOST_TEST_FAULTS)
	if (AGP::Testing::ConsumeRendererHostFault(AGP::Testing::RendererHostFault::BeforeMeshBufferPreparation))
	{
		return false;
	}
#endif

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
