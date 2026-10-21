#include "GameFramework.pch.h"
#include "GameFramework/Components/CameraComponent.h"
#include "GameFramework/World/Actor.h"
#include "Maths.hpp"
#include <cmath>
#include <stdexcept>

namespace
{
	constexpr float MinimumFieldOfViewDegrees = 0.0f;
	constexpr float MaximumFieldOfViewDegrees = 180.0f;
	constexpr float ParallelAxisToleranceSquared = 1.0e-8f;
	constexpr float WorldUpSelectionThreshold = 0.9f;
}

CameraComponent::CameraComponent()
{
	for (size_t i = 0; i < DirectionalLightComponent::ShadowCascadeCount; ++i)
	{
		myCascadeSplits[i] = DefaultCascadeSplits[i];
	}
	myCamera = Camera(DefaultFieldOfView, DefaultNearPlane, DefaultFarPlane, myCascadeSplits, DefaultAspectRatio);
}

CameraComponent::CameraComponent(float aHorizontalFieldOfViewDegrees, float aNearPlane, float aFarPlane,
                                 const CommonUtilities::Vector2u& aResolution)
    : CameraComponent()
{
	for (size_t i = 0; i < DirectionalLightComponent::ShadowCascadeCount; ++i)
	{
		myCascadeSplits[i] = DefaultCascadeSplits[i];
	}

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

std::array<CU::Vector3f, 8> CameraComponent::GetFrustumCorners() const
{
	return GetFrustumCorners(myNearPlane, myFarPlane);
}

std::array<CU::Vector3f, 8> CameraComponent::GetFrustumCorners(float aNearPlane, float aFarPlane) const
{
	const CU::Vector3f position = GetOwner()->GetTransform().GetWorldPosition();
	const CU::Vector3f forward = GetOwner()->GetTransform().GetLocalForward();
	const CU::Vector3f right = GetOwner()->GetTransform().GetLocalRight();
	const CU::Vector3f up = GetOwner()->GetTransform().GetLocalUp();
	const float tanHalfFov = std::tan(myHorizontalFieldOfViewRadians * 0.5f);

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
