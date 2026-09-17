#pragma once

#include "GameFramework/Components/SceneComponent.h"
#include "Matrix.hpp"
#include "Vector.hpp"

#include <cstdint>

enum class LightType : uint32_t
{
	Directional = 0,
	Point = 1,
	Spot = 2,
};

// Gameplay-facing light properties; position/direction come from its resolved spatial transform.
// Games set these values during callbacks, and snapshot extraction copies them for
// the renderer. Game code does not schedule shadow work or build GPU light buffers.
class LightComponent : public SceneComponent
{
public:
	LightType GetLightType() const;

	const CU::Vector3f& GetColor() const;
	void SetColor(const CU::Vector3f& aColor);

	float GetIntensity() const;
	void SetIntensity(float anIntensity);

	float GetRadius() const;
	void SetRadius(float aRadius);

	float GetInnerCone() const;
	float GetOuterCone() const;
	void SetConeAnglesDegrees(float anInnerConeDegrees, float anOuterConeDegrees);

	CU::Vector3f GetWorldPosition() const;
	CU::Vector3f GetWorldDirection() const;

protected:
	explicit LightComponent(LightType aType);

private:
	LightType myType;
	CU::Vector3f myColor = CU::Vector3f::One;
	float myIntensity = 1.0f;
	float myRadius = 1000.0f;
	float myInnerCone = 0.349066f;
	float myOuterCone = 0.610865f;
};

class DirectionalLightComponent final : public LightComponent
{
public:
	DirectionalLightComponent();
};

class PointLightComponent final : public LightComponent
{
public:
	PointLightComponent();
};

class SpotLightComponent final : public LightComponent
{
public:
	SpotLightComponent();
};
