#pragma once

#include "Matrix.hpp"
#include "Vector.hpp"

#include <array>

struct LightBuffer
{
	struct Light
	{
		CU::Vector3f Color = CU::Vector3f::One;
		float Intensity = 0.0f;

		CU::Vector3f Position = CU::Vector3f::Zero;
		unsigned Type = 0;

		CU::Vector3f Direction = CU::Vector3f::UnitZ;
		float InnerCone = 0.0f;

		float OuterCone = 0.0f;
		float Radius = 1.0f;
		CU::Vector2f __padding = CU::Vector2f::Zero;
	};

	static constexpr unsigned MaxLights = 8;

	std::array<Light, MaxLights> Lights = {};
	unsigned NumActiveLights = 0;
	CU::Vector3f __padding = CU::Vector3f::Zero;
};
