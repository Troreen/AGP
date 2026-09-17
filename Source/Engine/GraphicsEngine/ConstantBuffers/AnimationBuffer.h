#pragma once
#include "Matrix.hpp"

#include <array>

struct AnimationBuffer
{
	std::array<CU::Matrix4f, 128> JointTransforms;
};
