#pragma once

#include "Quaternion.hpp"

namespace ModelViewerOrientation
{
	// Upright orientation relative to the actor's parent: yaw around parent up,
	// then pitch around the camera's yawed right axis. With row-vector matrices,
	// yaw * pitch quaternions produce R_pitch * R_yaw.
	inline CommonUtilities::Quaternion<float> CreateUprightRotation(float yawRadians, float pitchRadians)
	{
		using Quaternion = CommonUtilities::Quaternion<float>;
		using Vector3f = CommonUtilities::Vector3f;

		const auto yaw = Quaternion::CreateFromAxisAngle(Vector3f::UnitY, yawRadians);
		const auto pitch = Quaternion::CreateFromAxisAngle(Vector3f::UnitX, pitchRadians);
		return (yaw * pitch).GetNormalized();
	}
}
