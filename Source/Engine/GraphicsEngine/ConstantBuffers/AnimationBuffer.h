#pragma once

#include <CommonUtilities/CommonUtilities.h>

struct AnimationBuffer
{
	static constexpr std::size_t MaxJointCount = 128;

    CU::Matrix4f JointAnimation[MaxJointCount];
};
