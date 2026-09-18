#pragma once

#include "Maths.hpp"
#include "Vector3.hpp"

namespace GameOrientation
{
	// Upright orientation: yaw around world up,
	// then pitch around the camera's yawed right axis. With row-vector matrices,
	// yaw * pitch quaternions produce R_pitch * R_yaw.
	inline CU::Vector3f CreateUprightRotation(float yawRadians, float pitchRadians)
	{
		return {CU::RadiansToDegrees(yawRadians), CU::RadiansToDegrees(pitchRadians), 0};
	}
}
