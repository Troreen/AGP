#pragma once

#include <CommonUtilities/CommonUtilities.h>

constexpr unsigned NUM_CASCADES = 3;
constexpr unsigned MAX_FORWARD_LIGHTS = 8;

struct Light
{
	CU::Matrix4f LightViewProjTexture[NUM_CASCADES];
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
	uint32_t NumActiveLights = 0;
	CU::Vector3f CascadeSplits;
};
