#pragma once
#include "Matrix.hpp"
#include "Vector3.hpp"

struct ObjectBuffer
{
	CU::Matrix4f World;
	unsigned HasSkinning = 0;
	CU::Vector3f __padding = CU::Vector3f::Zero;
};
