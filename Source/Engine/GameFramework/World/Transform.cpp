#include "GameFramework/World/Transform.h"
#include "Maths.hpp"
#include "Quaternion.hpp"

namespace
{
	bool Finite(const CU::Vector3f& value)
	{
		return CU::IsFinite(value.x) && CU::IsFinite(value.y) && CU::IsFinite(value.z);
	}

	bool Finite(const CU::Matrix4f& value)
	{
		for (int row = 1; row <= 4; ++row)
		{
			for (int column = 1; column <= 4; ++column)
			{
				if (!CU::IsFinite(value(row, column)))
				{
					return false;
				}
			}
		}
		return true;
	}

	// Transform axes may contain scale, so callers should only receive unit directions.
	CU::Vector3f Direction(const CU::Vector3f& value)
	{
		return CU::NormalizeSafe(value);
	}

	CU::Matrix4f MakeMatrix(const TransformData& data)
	{
		const CU::Quaternion<float> yaw =
			CU::Quaternion<float>::CreateFromAxisAngle(CU::Vector3f::UnitY, CU::DegreesToRadians(data.RotationDegrees.x));
		const CU::Quaternion<float> pitch =
			CU::Quaternion<float>::CreateFromAxisAngle(CU::Vector3f::UnitX, CU::DegreesToRadians(data.RotationDegrees.y));
		const CU::Quaternion<float> roll =
			CU::Quaternion<float>::CreateFromAxisAngle(CU::Vector3f::UnitZ, CU::DegreesToRadians(data.RotationDegrees.z));
		const CU::Matrix4f rotationMatrix = (yaw * pitch * roll).GetNormalized().ToMatrix4x4();
		return CU::CreateScale(data.Scale) * rotationMatrix * CU::CreateTranslation(data.Position);
	}
}

bool Transform::SetData(const TransformData& data)
{
	if (!Finite(data.Position) || !Finite(data.RotationDegrees) || !Finite(data.Scale))
	{
		return false;
	}
	const CU::Matrix4f matrix = MakeMatrix(data);
	if (!Finite(matrix))
	{
		return false;
	}
	myData = data;
	myLocalMatrix = matrix;
	return true;
}

bool Transform::SetLocalPosition(const CommonUtilities::Vector3f& position)
{
	TransformData data = myData;
	data.Position = position;
	return SetData(data);
}

bool Transform::SetLocalRotationDegrees(float yaw, float pitch, float roll)
{
	return SetLocalRotationDegrees({yaw, pitch, roll});
}

bool Transform::SetLocalRotationDegrees(const CommonUtilities::Vector3f& rotation)
{
	TransformData data = myData;
	data.RotationDegrees = rotation;
	return SetData(data);
}

bool Transform::SetLocalScale(const CommonUtilities::Vector3f& scale)
{
	TransformData data = myData;
	data.Scale = scale;
	return SetData(data);
}

CommonUtilities::Vector3f Transform::GetLocalRight() const
{
	return Direction({myLocalMatrix(1, 1), myLocalMatrix(1, 2), myLocalMatrix(1, 3)});
}

CommonUtilities::Vector3f Transform::GetLocalUp() const
{
	return Direction({myLocalMatrix(2, 1), myLocalMatrix(2, 2), myLocalMatrix(2, 3)});
}

CommonUtilities::Vector3f Transform::GetLocalForward() const
{
	return Direction({myLocalMatrix(3, 1), myLocalMatrix(3, 2), myLocalMatrix(3, 3)});
}
