#pragma once

#include <array>
#include <cstddef>
#include <cstdlib>
#include <string_view>

#include "Vector3.hpp"

namespace BenchmarkScene
{
	struct PrimitivePlacement
	{
		const char* MeshName = "Cube";
		CommonUtilities::Vector3f Position;
		CommonUtilities::Vector3f RotationDegrees;
		CommonUtilities::Vector3f Scale;
	};

	struct PointLightPlacement
	{
		CommonUtilities::Vector3f Position;
		CommonUtilities::Vector3f Color;
		float Intensity = 0.0f;
		float Radius = 0.0f;
	};

	constexpr size_t BusyGridWidth = 64;
	constexpr size_t BusyGridHeight = 64;
	constexpr size_t BusyPrimitiveCount = BusyGridWidth * BusyGridHeight;
	constexpr size_t BusyPointLightCount = 96;
	constexpr size_t BusyVisiblePointLightCount = 5;
	constexpr float BusyFloorScale = 40000.0f;
	static_assert(BusyPrimitiveCount == 4096);
	static_assert(BusyVisiblePointLightCount <= BusyPointLightCount);

	inline bool IsBusyScenario()
	{
		char scenario[16] = {};
		size_t requiredSize = 0;
		if (getenv_s(&requiredSize, scenario, sizeof(scenario), "AGP_BENCHMARK_SCENARIO") != 0)
		{
			return false;
		}
		return requiredSize > 0 && std::string_view(scenario) == "busy";
	}

	inline std::array<PrimitivePlacement, BusyPrimitiveCount> BuildBusyPrimitivePlacements()
	{
		constexpr std::array<const char*, 4> meshNames = {
			"Cube",
			"Pyramid",
			"Sphere",
			"Torus"
		};

		std::array<PrimitivePlacement, BusyPrimitiveCount> placements;
		for (size_t row = 0; row < BusyGridHeight; ++row)
		{
			for (size_t column = 0; column < BusyGridWidth; ++column)
			{
				const size_t index = row * BusyGridWidth + column;
				float x = (static_cast<float>(column) - 31.5f) * 170.0f;
				if (column < 24)
				{
					x -= 18000.0f;
				}
				else if (column >= 40)
				{
					x += 18000.0f;
				}

				const float z = 80.0f + static_cast<float>(row) * 140.0f;
				const float uniformScale = 22.0f + static_cast<float>((row + column) % 4) * 5.0f;
				placements[index] = {
					.MeshName = meshNames[(row + column) % meshNames.size()],
					.Position = { x, uniformScale, z },
					.RotationDegrees = {
						static_cast<float>((index * 17) % 360),
						static_cast<float>((index * 37) % 360),
						0.0f
					},
					.Scale = { uniformScale, uniformScale, uniformScale }
				};
			}
		}
		return placements;
	}

	inline std::array<PointLightPlacement, BusyPointLightCount> BuildBusyPointLightPlacements()
	{
		const std::array<CommonUtilities::Vector3f, 6> colors = {
			CommonUtilities::Vector3f{ 1.0f, 0.28f, 0.12f },
			CommonUtilities::Vector3f{ 0.2f, 0.55f, 1.0f },
			CommonUtilities::Vector3f{ 0.25f, 1.0f, 0.45f },
			CommonUtilities::Vector3f{ 0.95f, 0.2f, 0.85f },
			CommonUtilities::Vector3f{ 1.0f, 0.75f, 0.2f },
			CommonUtilities::Vector3f{ 0.25f, 0.95f, 1.0f }
		};

		std::array<PointLightPlacement, BusyPointLightCount> placements;
		for (size_t index = 0; index < BusyVisiblePointLightCount; ++index)
		{
			placements[index] = {
				.Position = {
					(static_cast<float>(index) - 2.0f) * 480.0f,
					180.0f + static_cast<float>(index % 2) * 90.0f,
					320.0f + static_cast<float>(index) * 480.0f
				},
				.Color = colors[index % colors.size()],
				.Intensity = 520.0f + static_cast<float>(index % 3) * 80.0f,
				.Radius = 820.0f
			};
		}

		for (size_t index = BusyVisiblePointLightCount; index < BusyPointLightCount; ++index)
		{
			const size_t offscreenIndex = index - BusyVisiblePointLightCount;
			const float side = offscreenIndex % 2 == 0 ? -1.0f : 1.0f;
			const float x = side * (22000.0f + static_cast<float>(offscreenIndex % 10) * 650.0f);
			const float z = 350.0f + static_cast<float>(offscreenIndex / 10) * 950.0f;
			placements[index] = {
				.Position = { x, 180.0f + static_cast<float>(offscreenIndex % 4) * 65.0f, z },
				.Color = colors[index % colors.size()],
				.Intensity = 280.0f + static_cast<float>(offscreenIndex % 5) * 45.0f,
				.Radius = 520.0f
			};
		}

		return placements;
	}
}
