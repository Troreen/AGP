#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace RenderItemRouting
{
	struct Passes { bool Opaque = false; bool Blended = false; };

	template<class Elements, class IsOpaque>
	Passes Classify(const Elements& elements, IsOpaque isOpaque)
	{
		Passes passes;
		for (const auto& element : elements)
		{
			if (isOpaque(element)) passes.Opaque = true;
			else passes.Blended = true;
		}
		return passes;
	}

	template<class Distance>
	void Sort(std::vector<size_t>& opaque, std::vector<size_t>& blended, Distance distance)
	{
		auto finiteDistance = [&](size_t index)
		{
			const auto value = distance(index);
			return std::isfinite(value) ? value : 0.0f;
		};
		std::stable_sort(opaque.begin(), opaque.end(),
			[&](size_t a, size_t b) { return finiteDistance(a) < finiteDistance(b); });
		std::stable_sort(blended.begin(), blended.end(),
			[&](size_t a, size_t b) { return finiteDistance(a) > finiteDistance(b); });
	}
}
