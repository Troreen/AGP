#include "LightComponent.h"

#include <algorithm>
#include <CommonUtilities/Maths.hpp>
#include <d3d11_1.h>
#include <GraphicsEngine/GraphicsEngine.h>

namespace
{
	constexpr float MaximumSpotConeDegrees = 89.0f;
}

const Texture* LightComponent::GetShadowMap() const
{
	if (myShadowMapSize.x == myShadowMapSize.y == 0)
	{
		return nullptr;
	}

	return &myShadowMap;
}

float LightComponent::GetIntensity() const
{
	return myIntensity * 10000;
}

void LightComponent::SetShadowMapSize(const CU::Vector2u& aSize)
{
	if (myShadowMapSize.x == aSize.x && myShadowMapSize.y == aSize.y)
	{
		return;
	}

	myShadowMapSize = aSize;
	if (myShadowMapSize.x == myShadowMapSize.y == 0)
	{
		myShadowMap = {};
		return;
	}

	std::string shadowMapName = "ShadowMap_SM";
	ensure(GraphicsEngine::Get().CreateShadowMap(shadowMapName, myShadowMapSize.x, myShadowMapSize.y, myShadowMap, GetLightType() == LightType::Point));
	myProjectionMatrix = CreateProjectionMatrix();
}

void LightComponent::SetIntensity(float anIntensity)
{
	myIntensity = (std::max)(0.0f, anIntensity);
}

void LightComponent::SetRadius(float aRadius)
{
	myRadius = (std::max)(1.0f, aRadius);
}

void LightComponent::SetConeAnglesDegrees(float anInnerConeDegrees, float anOuterConeDegrees)
{
	const float innerDegrees = CU::Clamp(anInnerConeDegrees, 0.0f, MaximumSpotConeDegrees);
	const float outerDegrees = CU::Clamp(anOuterConeDegrees, innerDegrees, MaximumSpotConeDegrees);
	myInnerCone = CU::Maths::DegreesToRadians(innerDegrees);
	myOuterCone = CU::Maths::DegreesToRadians(outerDegrees);
}

const CU::Matrix4f LightComponent::CreateProjectionMatrix() const
{
	return CU::Matrix4f::Identity;
}

// ======================
// Directional Light
// ======================

DirectionalLightComponent::DirectionalLightComponent()
{
	SetIntensity(10.0f);
}

float DirectionalLightComponent::GetIntensity() const
{
	return myIntensity;
}

const Texture* DirectionalLightComponent::GetShadowCascadeMap(size_t aIndex) const
{
	const CU::Vector2u* shadowMapSize = aIndex > 0 ? &myShadowCascadeMapSizes[aIndex - 1] : &myShadowMapSize;
	if (shadowMapSize->x == shadowMapSize->y == 0)
	{
		return nullptr;
	}

	const Texture* shadowMap = aIndex > 0 ? &myShadowCascadeMaps[aIndex - 1] : &myShadowMap;
	return shadowMap;
}

void DirectionalLightComponent::SetShadowCascadeMapSize(const CU::Vector2u& aSize, size_t aIndex)
{
	CU::Vector2u* shadowMapSize = aIndex > 0 ? &myShadowCascadeMapSizes[aIndex - 1] : &myShadowMapSize;
	if (shadowMapSize->x == aSize.x && shadowMapSize->y == aSize.y)
	{
		return;
	}

	*shadowMapSize = aSize;
	if (shadowMapSize->x == shadowMapSize->y == 0)
	{
		myShadowMap = {};
		return;
	}

	const std::string shadowMapName = std::format("Cascade {}_ShadowMap", std::to_string(aIndex));
	Texture* shadowMap = aIndex > 0 ? &myShadowCascadeMaps[aIndex - 1] : &myShadowMap;
	ensure(GraphicsEngine::Get().CreateShadowMap(shadowMapName, shadowMapSize->x, shadowMapSize->y, *shadowMap, false));
}

const CU::Matrix4f DirectionalLightComponent::CreateProjectionMatrix() const
{
	return CU::Matrix4f::CreateOrthographicProjection(static_cast<float>(myShadowMapSize.x), static_cast<float>(myShadowMapSize.y), 1.0f, 20000.0f);
}

// ======================
// Point Light
// ======================

PointLightComponent::PointLightComponent()
{
	SetIntensity(800.0f);
	SetRadius(800.0f);
}

const CU::Matrix4f PointLightComponent::CreateProjectionMatrix() const
{
	return CU::Matrix4f::CreatePerspectiveProjection(
		static_cast<float>(myShadowMapSize.x),
		static_cast<float>(myShadowMapSize.y),
		90 * CommonUtilities::Math::DEGREES_TO_RADIANS<float>,
		1.0f,
		myRadius);
}

// ======================
// Spot Light
// ======================

SpotLightComponent::SpotLightComponent()
{
	SetIntensity(1200.0f);
	SetRadius(1000.0f);
	SetConeAnglesDegrees(20.0f, 35.0f);
}

const CU::Matrix4f SpotLightComponent::CreateProjectionMatrix() const
{
	return CU::Matrix4f::CreatePerspectiveProjection(static_cast<float>(myShadowMapSize.x), static_cast<float>(myShadowMapSize.y), myOuterCone * 2, 1.0f, myRadius);
}
