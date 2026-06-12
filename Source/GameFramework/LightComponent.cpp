#include "LightComponent.h"

#include "Actor.h"
#include "Maths.hpp"

#include <algorithm>

LightComponent::LightComponent(LightType aType)
	: myType(aType)
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
	const float innerDegrees = std::clamp(anInnerConeDegrees, 0.0f, 89.0f);
	const float outerDegrees = std::clamp(anOuterConeDegrees, innerDegrees, 89.0f);
	myInnerCone = CU::Maths::DegreesToRadians(innerDegrees);
	myOuterCone = CU::Maths::DegreesToRadians(outerDegrees);
}

CU::Vector3f LightComponent::GetWorldPosition() const
{
	const Actor* owner = GetOwner();
	if (owner == nullptr)
	{
		return CU::Vector3f::Zero;
	}

	const CU::Matrix4f world = owner->GetTransform().GetWorldMatrix();
	return { world(4, 1), world(4, 2), world(4, 3) };
}

CU::Vector3f LightComponent::GetWorldDirection() const
{
	const Actor* owner = GetOwner();
	if (owner == nullptr)
	{
		return CU::Vector3f::UnitZ;
	}

	CU::Vector3f direction = owner->GetTransform().GetForward();
	if (direction.LengthSqr() <= 0.000001f)
	{
		return CU::Vector3f::UnitZ;
	}

	return direction.GetNormalized();
}

DirectionalLightComponent::DirectionalLightComponent()
	: LightComponent(LightType::Directional)
{
	SetIntensity(10.0f);
}

PointLightComponent::PointLightComponent()
	: LightComponent(LightType::Point)
{
	SetIntensity(800.0f);
	SetRadius(800.0f);
}

SpotLightComponent::SpotLightComponent()
	: LightComponent(LightType::Spot)
{
	SetIntensity(1200.0f);
	SetRadius(1000.0f);
	SetConeAnglesDegrees(20.0f, 35.0f);
}
