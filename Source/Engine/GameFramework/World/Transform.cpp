#include "GameFramework.pch.h"
#include "GameFramework/World/Transform.h"
#include "Maths.hpp"

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
	CU::Vector3f Direction(const CU::Vector3f& aValue)
	{
		return CU::NormalizeSafe(aValue);
	}
}

bool Transform::SetMatrix(const CommonUtilities::Matrix4x4<float>& aMatrix)
{
	CommonUtilities::Vector3f scale;
	CommonUtilities::Vector3f position;
	CommonUtilities::Quaternion<float> rotation;
	if (!CommonUtilities::DecomposeSRT(aMatrix, scale, rotation, position))
	{
		return false;
	}

	myPosition = position;
	myRotation = rotation;
	myScale = scale;

	myLocalMatrix = aMatrix;

	return true;
}

CommonUtilities::Vector3f Transform::GetLocalRotationDegrees() const
{
	const CommonUtilities::Vector3f eulerInRadians = myRotation.GetEulerAngles();

	const CommonUtilities::Vector3f eulerInDegrees =
	{
		CommonUtilities::RadiansToDegrees(eulerInRadians.x),
		CommonUtilities::RadiansToDegrees(eulerInRadians.y),
		CommonUtilities::RadiansToDegrees(eulerInRadians.z)
	};

	return eulerInDegrees;
}

CommonUtilities::Vector3f Transform::GetLocalRotationRadians() const
{
	CommonUtilities::Vector3f eulerInRadians = myRotation.GetEulerAngles();

	return eulerInRadians;
}

void Transform::SetLocalPosition(const CommonUtilities::Vector3f& aPosition)
{
	myPosition = aPosition;
	myIsDirty = true;
}

void Transform::SetLocalRotationDegrees(float aPitch, float aYaw, float aRoll)
{
	SetLocalRotationDegrees({ aPitch, aYaw, aRoll });
}

void Transform::SetLocalRotationDegrees(const CommonUtilities::Vector3f& aRotation)
{
	const CommonUtilities::Vector3f& rotationInRadians =
	{
		CommonUtilities::DegreesToRadians(aRotation.x),
		CommonUtilities::DegreesToRadians(aRotation.y),
		CommonUtilities::DegreesToRadians(aRotation.z)
	};

	const CommonUtilities::Quaternion<float>& rotation = CommonUtilities::Quaternion<float>::CreateFromEulerAngles(rotationInRadians);

	if (rotation.LengthSqr() < 0.000001f)
	{
		return;
	}

	myRotation = rotation;
	myIsDirty = true;
}

void Transform::SetLocalRotation(const CommonUtilities::Vector3f& aRotation)
{
	const CommonUtilities::Quaternion<float>& rotation = CommonUtilities::Quaternion<float>::CreateFromEulerAngles(aRotation);

	if (rotation.LengthSqr() < 0.000001f)
	{
		return;
	}

	myRotation = rotation;
	myIsDirty = true;
}

void Transform::SetLocalRotation(const CommonUtilities::Quaternion<float>& aRotation)
{
	if (aRotation.LengthSqr() < 0.000001f)
	{
		return;
	}

	myRotation = aRotation;
	myIsDirty = true;
}

void Transform::SetLocalScale(const CommonUtilities::Vector3f& aScale)
{
	myScale = aScale;
	myIsDirty = true;
}

const CommonUtilities::Matrix4f& Transform::GetLocalMatrix() const
{
	if (myIsDirty)
	{
		RecalculateMatrix();
	}

	return myLocalMatrix;

}

const CommonUtilities::Matrix4f& Transform::GetWorldMatrix() const
{
	if (myIsDirty)
	{
		RecalculateMatrix();
	}

	return myLocalMatrix;
}

CommonUtilities::Vector3f Transform::GetLocalRight() const
{
	return Direction({ myLocalMatrix(1, 1), myLocalMatrix(1, 2), myLocalMatrix(1, 3) });
}

CommonUtilities::Vector3f Transform::GetLocalUp() const
{
	return Direction({ myLocalMatrix(2, 1), myLocalMatrix(2, 2), myLocalMatrix(2, 3) });
}

void Transform::RecalculateMatrix() const
{
	const CU::Matrix4f rotationMatrix(myRotation.GetRotationMatrix());

	const CommonUtilities::Matrix4f newMatrix = CU::CreateScale(myScale) * rotationMatrix * CU::CreateTranslation(myPosition);

	myLocalMatrix = newMatrix;

	myIsDirty = false;
}

CommonUtilities::Vector3f Transform::GetLocalForward() const
{
	return Direction({ myLocalMatrix(3, 1), myLocalMatrix(3, 2), myLocalMatrix(3, 3) });
}
