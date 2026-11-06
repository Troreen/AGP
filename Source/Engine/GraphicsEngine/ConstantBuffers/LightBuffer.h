#pragma once

#include <CommonUtilities/CommonUtilities.h>
#include <GameFramework/Components/LightComponent.h>

constexpr unsigned MAX_FORWARD_LIGHTS = 8;

struct Light
{
	CU::Matrix4f LightViewProjTexture[DirectionalLightComponent::ShadowCascadeCount];
	CU::Vector3f Color;
	float Intensity = 0;
	CU::Vector3f Position;
	uint32_t Type = 0;
	CU::Vector3f Direction;
	float InnerCone = 0;
	float OuterCone = 0;
	float Radius = 0;
	unsigned CastShadows = false;
	float __padding = 0;
};

struct LightBuffer
{
	Light Lights[MAX_FORWARD_LIGHTS];
	CU::Vector4f CascadeSplits;
	uint32_t NumActiveLights = 0;
	CU::Vector3f AmbientLightColor;
	float AmbientLightIntensity = 1;
	float EV100Exposure = 2.0f;
	CU::Vector2f __padding;
};
