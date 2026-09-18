#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace RenderItemRouting
{
	struct Passes
	{
		bool Opaque = false;
		bool Blended = false;
	};

	template <typename Elements, typename IsOpaque> Passes Classify(const Elements& elements, IsOpaque isOpaque)
	{
		Passes passes;
		for (const auto& element : elements)
		{
			if (isOpaque(element))
			{
				passes.Opaque = true;
			}
			else
			{
				passes.Blended = true;
			}
		}
		return passes;
	}

	template <typename Distance> void Sort(std::vector<size_t>& opaque, std::vector<size_t>& blended, Distance distance)
	{
		auto finiteDistance = [&distance](size_t index)
		{
			const auto value = distance(index);
			return std::isfinite(value) ? value : 0.0f;
		};

		// Preserve submission order when objects compare equally to avoid frame-to-frame flicker.
		std::stable_sort(opaque.begin(), opaque.end(), [&finiteDistance](size_t a, size_t b)
		{
			return finiteDistance(a) < finiteDistance(b);
		});
		std::stable_sort(blended.begin(), blended.end(), [&finiteDistance](size_t a, size_t b)
		{
			return finiteDistance(a) > finiteDistance(b);
		});
	}
}
