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

CameraComponent::CameraComponent() : myCamera(DefaultFieldOfView, DefaultNearPlane, DefaultFarPlane, CommonUtilities::Vector2u{1280, 720})
{
}

CameraComponent::CameraComponent(float aHorizontalFieldOfViewDegrees, float aNearPlane, float aFarPlane,
                                 const CommonUtilities::Vector2u& aResolution)
    : CameraComponent()
{
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
	CommonUtilities::Camera3D candidate(aHorizontalFieldOfViewDegrees, aNearPlane, aFarPlane, aResolution);
	const CommonUtilities::Matrix4f projectionMatrix = candidate.GetProjectionMatrix();
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
	SyncCameraToOwner();
	return true;
}

void CameraComponent::SyncCameraToOwner()
{
	const Actor* owner = GetOwner();
	if (owner == nullptr)
	{
		return;
	}

	// Build an orthonormal camera basis from the composed pose. Inherited scale
	// affects placement, but never projection or view-axis lengths.
	const CommonUtilities::Matrix4f worldMatrix = GetWorldMatrix();
	const CommonUtilities::Vector3f forward = GetWorldDirection();
	CommonUtilities::Vector3f up{worldMatrix(2, 1), worldMatrix(2, 2), worldMatrix(2, 3)};
	CommonUtilities::Vector3f right = up.Cross(forward);
	if (right.LengthSqr() < ParallelAxisToleranceSquared)
	{
		const CommonUtilities::Vector3f referenceAxis = std::abs(forward.y) < WorldUpSelectionThreshold
			? CommonUtilities::Vector3f::UnitY
			: CommonUtilities::Vector3f::UnitX;
		right = referenceAxis.Cross(forward);
	}
	right.Normalize();
	up = CU::NormalizeSafe(forward.Cross(right), CU::Vector3f::UnitY);
	CommonUtilities::Matrix4f rigidWorldMatrix;
	rigidWorldMatrix(1, 1) = right.x;
	rigidWorldMatrix(1, 2) = right.y;
	rigidWorldMatrix(1, 3) = right.z;
	rigidWorldMatrix(2, 1) = up.x;
	rigidWorldMatrix(2, 2) = up.y;
	rigidWorldMatrix(2, 3) = up.z;
	rigidWorldMatrix(3, 1) = forward.x;
	rigidWorldMatrix(3, 2) = forward.y;
	rigidWorldMatrix(3, 3) = forward.z;
	rigidWorldMatrix(4, 1) = worldMatrix(4, 1);
	rigidWorldMatrix(4, 2) = worldMatrix(4, 2);
	rigidWorldMatrix(4, 3) = worldMatrix(4, 3);
	myCamera.SetWorldMatrix(rigidWorldMatrix);
}
