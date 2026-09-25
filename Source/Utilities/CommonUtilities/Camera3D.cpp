#include "Camera3D.hpp"

#include <array>

void CommonUtilities::Camera3D::SetPerspective(float aFieldOfViewDegrees, float aAspectRatio, float aNearPlane, float aFarPlane)
{
	myProjectionType = ProjectionType::Perspective;
	myFieldOfViewRadians = aFieldOfViewDegrees * Math::DEGREES_TO_RADIANS<float>;
	myAspectRatio = aAspectRatio;
	myNearPlane = aNearPlane;
	myFarPlane = aFarPlane;
}

void CommonUtilities::Camera3D::SetOrthographic(float aWidth, float aHeight, float aNearPlane, float aFarPlane)
{
	SetOrthographic(-aWidth * 0.5f, aWidth * 0.5f, -aHeight * 0.5f, aHeight * 0.5f, aNearPlane, aFarPlane);
}

void CommonUtilities::Camera3D::SetOrthographic(float aLeft, float aRight, float aBottom, float aTop, float aNearPlane, float aFarPlane)
{
	myProjectionType = ProjectionType::Orthographic;
	myOrthoLeft = aLeft;
	myOrthoRight = aRight;
	myOrthoBottom = aBottom;
	myOrthoTop = aTop;
	myNearPlane = aNearPlane;
	myFarPlane = aFarPlane;
}

void CommonUtilities::Camera3D::SetPosition(const Vector3<float>& aPosition)
{
	myWorldMatrix(4, 1) = aPosition.x;
	myWorldMatrix(4, 2) = aPosition.y;
	myWorldMatrix(4, 3) = aPosition.z;
}

void CommonUtilities::Camera3D::SetRotationDegrees(const Vector3<float>& aRotation)
{
	const CommonUtilities::Vector3f position = GetPosition();
	myWorldMatrix = Matrix4f::CreateRotationAroundY(aRotation.x * Math::DEGREES_TO_RADIANS<float>) *
		Matrix4f::CreateRotationAroundX(aRotation.y * Math::DEGREES_TO_RADIANS<float>) *
		Matrix4f::CreateRotationAroundZ(aRotation.z * Math::DEGREES_TO_RADIANS<float>);
	SetPosition(position);
}

CommonUtilities::Matrix4f CommonUtilities::Camera3D::GetProjectionMatrix() const
{
	Matrix4f projection;

	if (myProjectionType == ProjectionType::Perspective)
	{
		const float f = 1.0f / std::tan(myFieldOfViewRadians * 0.5f);
		projection(1, 1) = f / myAspectRatio;
		projection(2, 2) = f;
		projection(3, 3) = myFarPlane / (myFarPlane - myNearPlane);
		projection(3, 4) = 1.0f;
		projection(4, 3) = (-myNearPlane * myFarPlane) / (myFarPlane - myNearPlane);
		projection(4, 4) = 0.0f;
	}
	else
	{
		const float rightMinusLeft = myOrthoRight - myOrthoLeft;
		const float topMinusBottom = myOrthoTop - myOrthoBottom;
		const float farMinusNear = myFarPlane - myNearPlane;

		projection(1, 1) = 2.0f / rightMinusLeft;
		projection(2, 2) = 2.0f / topMinusBottom;
		projection(3, 3) = 1.0f / farMinusNear;
		projection(4, 1) = -(myOrthoRight + myOrthoLeft) / rightMinusLeft;
		projection(4, 2) = -(myOrthoTop + myOrthoBottom) / topMinusBottom;
		projection(4, 3) = -myNearPlane / farMinusNear;
	}

	return projection;
}

std::array<CommonUtilities::Vector3f, 8> CommonUtilities::Camera3D::GetFrustumCorners() const
{
	return GetFrustumCorners(myNearPlane, myFarPlane);
}

std::array<CommonUtilities::Vector3f, 8> CommonUtilities::Camera3D::GetFrustumCorners(float aNearPlane, float aFarPlane) const
{
	namespace CU = CommonUtilities;

	const CU::Vector3f position = GetPosition();
	const CU::Vector3f forward = GetForward();
	const CU::Vector3f right = GetRight();
	const CU::Vector3f up = GetUp();
	const float tanHalfFov = std::tan(GetFieldOfViewRadians() * 0.5f);

	const float nearHeight = 2.0f * tanHalfFov * aNearPlane;
	const float nearWidth = nearHeight * GetAspectRatio();
	const float farHeight = 2.0f * tanHalfFov * aFarPlane;
	const float farWidth = farHeight * GetAspectRatio();

	const CU::Vector3f nearCenter = position + forward * aNearPlane;
	const CU::Vector3f farCenter = position + forward * aFarPlane;

	return { nearCenter - right * (nearWidth * 0.5f) + up * (nearHeight * 0.5f),
			nearCenter + right * (nearWidth * 0.5f) + up * (nearHeight * 0.5f),
			nearCenter + right * (nearWidth * 0.5f) - up * (nearHeight * 0.5f),
			nearCenter - right * (nearWidth * 0.5f) - up * (nearHeight * 0.5f),
			farCenter - right * (farWidth * 0.5f) + up * (farHeight * 0.5f),
			farCenter + right * (farWidth * 0.5f) + up * (farHeight * 0.5f),
			farCenter + right * (farWidth * 0.5f) - up * (farHeight * 0.5f),
			farCenter - right * (farWidth * 0.5f) - up * (farHeight * 0.5f) };
}

void CommonUtilities::Camera3D::LookAt(const Vector3<float>& aTarget)
{
	const Vector3<float> position = GetPosition();
	Vector3<float> forward = (aTarget - position).GetNormalized();
	if (forward.LengthSqr() == 0.0f)
	{
		return;
	}

	const float yaw = std::atan2(forward.x, forward.z);
	const float pitch = -std::asin(forward.y);
	SetRotationDegrees({ yaw * Math::RADIANS_TO_DEGREES<float>, pitch * Math::RADIANS_TO_DEGREES<float>, 0 });
}

CommonUtilities::Ray<float> CommonUtilities::Camera3D::ScreenPointToRay(const Vector2<float>& aNormalizedScreenPos) const
{
	const Vector3<float> origin = GetPosition();

	if (myProjectionType == ProjectionType::Orthographic)
	{
		const float ndcX = (aNormalizedScreenPos.x * 2.0f) - 1.0f;
		const float ndcY = 1.0f - (aNormalizedScreenPos.y * 2.0f);

		const Vector3<float> right = GetRight();
		const Vector3<float> up = GetUp();

		const Vector3<float> offset =
			right * (ndcX * (myOrthoRight - myOrthoLeft) * 0.5f) +
			up * (ndcY * (myOrthoTop - myOrthoBottom) * 0.5f);

		return Ray<float>(origin + offset, GetForward());
	}

	const float tanHalfFov = std::tan(myFieldOfViewRadians * 0.5f);
	const float ndcX = (aNormalizedScreenPos.x * 2.0f) - 1.0f;
	const float ndcY = 1.0f - (aNormalizedScreenPos.y * 2.0f);

	Vector3<float> localDir(
		ndcX * tanHalfFov * myAspectRatio,
		ndcY * tanHalfFov,
		1.0f);

	localDir = localDir.GetNormalized();

	Vector3<float> worldDir = GetRight() * localDir.x + GetUp() * localDir.y + GetForward() * localDir.z;
	worldDir.Normalize();

	return Ray<float>(origin, worldDir);
}

CommonUtilities::Vector3<float> CommonUtilities::Camera3D::WorldToScreenPoint(const Vector3<float>& aWorldPos) const
{
	const Matrix4f viewProjection = GetViewProjectionMatrix();
	Vector4<float> clip = Vector4<float>(aWorldPos.x, aWorldPos.y, aWorldPos.z, 1.0f) * viewProjection;

	if (clip.w == 0.0f)
	{
		return Vector3<float>::Zero;
	}

	const float invW = 1.0f / clip.w;
	const Vector3<float> ndc(clip.x * invW, clip.y * invW, clip.z * invW);

	return Vector3<float>(
		(ndc.x + 1.0f) * 0.5f,
		(1.0f - ndc.y) * 0.5f,
		ndc.z);
}
