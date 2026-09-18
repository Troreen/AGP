#include "GameFramework/Components/LightComponent.h"

#include "GameFramework/World/Actor.h"
#include "Maths.hpp"

#include <algorithm>

namespace
{
	constexpr float MaximumSpotConeDegrees = 89.0f;
}

LightComponent::LightComponent(LightType aType) : myType(aType)
{
}

LightType LightComponent::GetLightType() const
{
	return myType;
}

const CU::Vector3f& LightComponent::GetColor() const
{
	return myColor;
}

void LightComponent::SetColor(const CU::Vector3f& aColor)
{
	myColor = aColor;
}

float LightComponent::GetIntensity() const
{
	return myIntensity;
}

void LightComponent::SetIntensity(float anIntensity)
{
	myIntensity = std::max(0.0f, anIntensity);
}

float LightComponent::GetRadius() const
{
	return myRadius;
}

void LightComponent::SetRadius(float aRadius)
{
	myRadius = std::max(1.0f, aRadius);
}

float LightComponent::GetInnerCone() const
{
	return myInnerCone;
}

float LightComponent::GetOuterCone() const
{
	return myOuterCone;
}

void LightComponent::SetConeAnglesDegrees(float anInnerConeDegrees, float anOuterConeDegrees)
{
	const float innerDegrees = CU::Clamp(anInnerConeDegrees, 0.0f, MaximumSpotConeDegrees);
	const float outerDegrees = CU::Clamp(anOuterConeDegrees, innerDegrees, MaximumSpotConeDegrees);
	myInnerCone = CU::Maths::DegreesToRadians(innerDegrees);
	myOuterCone = CU::Maths::DegreesToRadians(outerDegrees);
}

CU::Vector3f LightComponent::GetWorldPosition() const
{
	return SceneComponent::GetWorldPosition();
}

CU::Vector3f LightComponent::GetWorldDirection() const
{
	return SceneComponent::GetWorldDirection();
}

DirectionalLightComponent::DirectionalLightComponent() : LightComponent(LightType::Directional)
{
	SetIntensity(10.0f);
}

PointLightComponent::PointLightComponent() : LightComponent(LightType::Point)
{
	SetIntensity(800.0f);
	SetRadius(800.0f);
}

SpotLightComponent::SpotLightComponent() : LightComponent(LightType::Spot)
{
	SetIntensity(1200.0f);
	SetRadius(1000.0f);
	SetConeAnglesDegrees(20.0f, 35.0f);
}
