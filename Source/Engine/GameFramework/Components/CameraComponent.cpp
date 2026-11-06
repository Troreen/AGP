#include "GameFramework.pch.h"
#include "GameFramework/Components/CameraComponent.h"
#include "GameFramework/World/Actor.h"
#include "Maths.hpp"

#include <cmath>
#include <CommonUtilities/Intersection.hpp>
#include <stdexcept>

namespace
{
	constexpr float MinimumFieldOfViewDegrees = 0.0f;
	constexpr float MaximumFieldOfViewDegrees = 180.0f;
	constexpr float ParallelAxisToleranceSquared = 1.0e-8f;
	constexpr float WorldUpSelectionThreshold = 0.9f;
}

bool Frustum::IsCulled(const CU::Spheref& aCullingSphere) const
{
	for (const CommonUtilities::Planef& plane : planes)
	{
		if (!plane.IsInside(aCullingSphere.GetCenter()) && !CU::IntersectionPlaneSphere(plane, aCullingSphere))
		{
			return true;
		}
	}

	return false;
}

CameraComponent::CameraComponent()
{
	SetCascadeSplits();

	myCamera = Camera(DefaultFieldOfView, DefaultNearPlane, DefaultFarPlane, myCascadeSplits, DefaultAspectRatio);
}

CameraComponent::CameraComponent(float aHorizontalFieldOfViewDegrees, float aNearPlane, float aFarPlane,
	const CommonUtilities::Vector2u& aResolution)
	: CameraComponent()
{
	myNearPlane = aNearPlane;
	myFarPlane = aFarPlane;

	SetCascadeSplits();

	if (!SetPerspective(aHorizontalFieldOfViewDegrees, aNearPlane, aFarPlane, aResolution))
	{
		throw std::invalid_argument("Invalid camera projection");
	}
}

bool CameraComponent::SetPerspective(float aHorizontalFieldOfViewDegrees, float aNearPlane, float aFarPlane,
	const CommonUtilities::Vector2u& aResolution)
{
	if (!CU::IsFinite(aHorizontalFieldOfViewDegrees) || aHorizontalFieldOfViewDegrees <= MinimumFieldOfViewDegrees ||
		aHorizontalFieldOfViewDegrees >= MaximumFieldOfViewDegrees || !CU::IsFinite(aNearPlane) || !CU::IsFinite(aFarPlane) ||
		aNearPlane <= 0 || aFarPlane <= aNearPlane || aResolution.x == 0 ||
		aResolution.y == 0)
	{
		return false;
	}

	float aspectRatio = static_cast<float>(aResolution.y) / static_cast<float>(aResolution.x);
	Camera candidate(aHorizontalFieldOfViewDegrees, aNearPlane, aFarPlane, myCascadeSplits, aspectRatio);
	const CommonUtilities::Matrix4f& projectionMatrix = candidate.GetProjection();
	for (int row = 1; row <= 4; ++row)
	{
		for (int column = 1; column <= 4; ++column)
		{
			if (!CU::IsFinite(projectionMatrix(row, column)))
			{
				return false;
			}
		}
	}

	myCamera = std::move(candidate);
	myHorizontalFieldOfViewRadians = aHorizontalFieldOfViewDegrees * CU::Math::DEGREES_TO_RADIANS<float>;
	myNearPlane = aNearPlane;
	myFarPlane = aFarPlane;
	myAspectRatio = aspectRatio;
	return true;
}

bool CameraComponent::SetResolution(const CommonUtilities::Vector2u& aResolution)
{
	return SetPerspective(GetFieldOfViewDegrees(), myNearPlane, myFarPlane, aResolution);
}

const Camera& CameraComponent::GetCamera() const
{
	return myCamera;
}

CU::Matrix4f CameraComponent::GetView() const
{
	return GetOwner()->GetTransform().GetWorldMatrix().GetFastInverse();
}

const CU::Matrix4f& CameraComponent::GetProjection() const
{
	return myCamera.GetProjection();
}

const CU::Matrix4f& CameraComponent::GetProjectionByCascadeIndex(size_t aIndex) const
{
	return myCamera.GetProjectionByCascadeIndex(aIndex);
}

float CameraComponent::GetCascadeSplit(size_t aIndex) const
{
	return myCascadeSplits[aIndex];
}

Frustum CameraComponent::GetFrustum() const
{
	return GetFrustum(myNearPlane, myFarPlane);
}

Frustum CameraComponent::GetFrustum(float aNearPlane, float aFarPlane) const
{
	return GetFrustum(myCamera.GetProjection(), GetOwner()->GetTransform().GetWorldMatrix(), aNearPlane, aFarPlane);
}

Frustum CameraComponent::GetFrustum(const CU::Matrix4f& aProjection, const CU::Matrix4f& aWorld, float aNearPlane, float aFarPlane)
{
	Frustum frustum;

	const CU::Vector3f right(aWorld(1, 1), aWorld(1, 2), aWorld(1, 3));
	const CU::Vector3f up(aWorld(2, 1), aWorld(2, 2), aWorld(2, 3));
	const CU::Vector3f forward(aWorld(3, 1), aWorld(3, 2), aWorld(3, 3));
	const CU::Vector3f position(aWorld(4, 1), aWorld(4, 2), aWorld(4, 3));

	const float inverseXScale = 1.0f / aProjection(1, 1);
	const float inverseYScale = 1.0f / aProjection(1, 1);

	if (aProjection(3, 4) == 0) // Orthographic perspective
	{
		frustum.left.InitWithPointAndNormal(position - right * inverseXScale, -right);
		frustum.right.InitWithPointAndNormal(position + right * inverseXScale, right);

		frustum.top.InitWithPointAndNormal(position + up * inverseYScale, up);
		frustum.bottom.InitWithPointAndNormal(position - up * inverseYScale, -up);

	}
	else
	{
		const CU::Vector3f leftNormal = (forward * inverseXScale) - right;
		const CU::Vector3f rightNormal = (forward * inverseXScale) + right;

		const CU::Vector3f topNormal = (forward * inverseYScale) + up;
		const CU::Vector3f bottomNormal = (forward * inverseYScale) - up;

		frustum.left.InitWithPointAndNormal(position, -leftNormal);
		frustum.right.InitWithPointAndNormal(position, -rightNormal);

		frustum.top.InitWithPointAndNormal(position, -topNormal);
		frustum.bottom.InitWithPointAndNormal(position, -bottomNormal);
	}

	const CU::Vector3f nearPoint = position + (forward * aNearPlane);
	const CU::Vector3f farPoint = position + (forward * aFarPlane);

	frustum.nearplane.InitWithPointAndNormal(nearPoint, -forward);
	frustum.farplane.InitWithPointAndNormal(farPoint, forward);

	return frustum;
}

void CameraComponent::LookAt(const CU::Vector3<float>& aTarget)
{
	const CU::Vector3<float> position = GetOwner()->GetTransform().GetWorldPosition();
	CU::Vector3<float> forward = (aTarget - position).GetNormalized();
	if (forward.LengthSqr() == 0.0f)
	{
		return;
	}

	const float yaw = std::atan2(forward.x, forward.z);
	const float pitch = -std::asin(forward.y);
	GetOwner()->GetTransform().SetLocalRotationDegrees({ yaw * CU::Math::RADIANS_TO_DEGREES<float>, pitch * CU::Math::RADIANS_TO_DEGREES<float>, 0 });
}

CU::Ray<float> CameraComponent::ScreenPointToRay(const CU::Vector2<float>& aScreenPos, const CU::Vector2<unsigned>& aClientSize) const
{
	Transform& transform = GetOwner()->GetTransform();
	const CU::Vector3<float> origin = transform.GetWorldPosition();

	// TODO: If adding orthographic projection to game, re-add.
	//if (myProjectionType == ProjectionType::Orthographic)
	//{
	//	const float ndcX = (aNormalizedScreenPos.x * 2.0f) - 1.0f;
	//	const float ndcY = 1.0f - (aNormalizedScreenPos.y * 2.0f);

	//	const CU::Vector3<float> right = transform.GetLocalRight();
	//	const CU::Vector3<float> up = transform.GetLocalUp();

	//	const CU::Vector3<float> offset =
	//		right * (ndcX * (myOrthoRight - myOrthoLeft) * 0.5f) +
	//		up * (ndcY * (myOrthoTop - myOrthoBottom) * 0.5f);

	//	return CU::Ray<float>(origin + offset, transform.GetLocalForward());
	//}

	CommonUtilities::Vector2f normalizedMousePos =
	{
		aScreenPos.x / static_cast<float>(aClientSize.x),
		aScreenPos.y / static_cast<float>(aClientSize.y)
	};

	const float tanHalfFov = std::tan(myHorizontalFieldOfViewRadians * 0.5f);
	const float ndcX = (normalizedMousePos.x * 2.0f) - 1.0f;
	const float ndcY = 1.0f - (normalizedMousePos.y * 2.0f);

	CU::Vector3<float> localDir(ndcX * tanHalfFov, ndcY * tanHalfFov * myAspectRatio, 1.0f);

	localDir = localDir.GetNormalized();

	CU::Vector3<float> worldDir = transform.GetLocalRight() * localDir.x + transform.GetLocalUp() * localDir.y + transform.GetLocalForward() * localDir.z;
	worldDir.Normalize();

	return CU::Ray<float>(origin, worldDir);
}

CU::Vector3<float> CameraComponent::WorldToScreenPoint(const CU::Vector3<float>& aWorldPos) const
{
	const CU::Matrix4f viewProjection = GetView() * GetProjection();
	CU::Vector4<float> clip = CU::Vector4<float>(aWorldPos.x, aWorldPos.y, aWorldPos.z, 1.0f) * viewProjection;

	if (clip.w == 0.0f)
	{
		return CU::Vector3<float>::Zero;
	}

	const float invW = 1.0f / clip.w;
	const CU::Vector3<float> ndc(clip.x * invW, clip.y * invW, clip.z * invW);

	return CU::Vector3<float>(
		(ndc.x + 1.0f) * 0.5f,
		(1.0f - ndc.y) * 0.5f,
		ndc.z);
}

void CameraComponent::SetCascadeSplits()
{
	constexpr float cascadeSplitLambdas[DirectionalLightComponent::ShadowCascadeCount] = { 0.9f, 0.7f, 0.65f, 0.6f };

	const float m = (float)(DirectionalLightComponent::ShadowCascadeCount);
	const float ratio = myFarPlane / myNearPlane;
	const float clipRange = myFarPlane - myNearPlane;

	for (size_t i = 0; i < DirectionalLightComponent::ShadowCascadeCount; ++i)
	{
		const float p = (i + 1) / m;
		const float log = myNearPlane * std::pow(ratio, p);
		const float uniform = myNearPlane + clipRange * p;
		myCascadeSplits[i] = cascadeSplitLambdas[i] * (log - uniform) + uniform;
	}
}
