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

	constexpr size_t BusyGridWidth = 16;
	constexpr size_t BusyGridHeight = 16;
	constexpr size_t BusyPrimitiveCount = BusyGridWidth * BusyGridHeight;
	static_assert(BusyPrimitiveCount == 256);

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
				float x = (static_cast<float>(column) - 7.5f) * 180.0f;
				if (column < 2)
				{
					x -= 2400.0f;
				}
				else if (column >= BusyGridWidth - 2)
				{
					x += 2400.0f;
				}

				const float z = 80.0f + static_cast<float>(row) * 120.0f;
				const float uniformScale = 32.0f + static_cast<float>((row + column) % 4) * 4.0f;
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
}
