#pragma once
#include "Matrix.hpp"

#include <array>
#include <cstddef>

struct AnimationBuffer
{
	static constexpr std::size_t MaxJointCount = 128;

	std::array<CU::Matrix4f, MaxJointCount> JointTransforms;
};
