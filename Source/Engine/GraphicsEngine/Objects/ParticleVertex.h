#pragma once

#include <CommonUtilities/CommonUtilities.h>

struct ParticleVertex
{
	CU::Vector4f Position;
	CU::Vector4f Color;
	CU::Vector3f Velocity;

	float Lifetime = 0;
	float Angle = 0;

	CU::Vector2f Size;
	CU::Vector4f ChannelMask;

	static const std::vector<VertexElementDesc> Description;
};
