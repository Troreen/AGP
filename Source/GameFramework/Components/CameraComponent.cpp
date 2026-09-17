#include "GameFramework/Components/CameraComponent.h"
#include <DirectXMath.h>

#include "GameFramework/World/Actor.h"
#include <cmath>
#include <stdexcept>

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
	if (!std::isfinite(aHorizontalFieldOfViewDegrees) || aHorizontalFieldOfViewDegrees <= 0 || aHorizontalFieldOfViewDegrees >= 180 ||
	    !std::isfinite(aNearPlane) || !std::isfinite(aFarPlane) || aNearPlane <= 0 || aFarPlane <= aNearPlane || aResolution.x == 0 ||
	    aResolution.y == 0)
	{
		return false;
	}
	CommonUtilities::Camera3D candidate(aHorizontalFieldOfViewDegrees, aNearPlane, aFarPlane, aResolution);
	const auto projection = candidate.GetProjectionMatrix();
	for (int row = 1; row <= 4; ++row)
	{
		for (int column = 1; column <= 4; ++column)
		{
			if (!std::isfinite(projection(row, column)))
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
	auto matrix = GetWorldMatrix();
	auto forward = GetWorldDirection();
	CommonUtilities::Vector3f up{matrix(2, 1), matrix(2, 2), matrix(2, 3)};
	auto right = up.Cross(forward);
	if (right.LengthSqr() < 1e-8f)
	{
		const auto reference = std::abs(forward.y) < .9f ? CommonUtilities::Vector3f::UnitY : CommonUtilities::Vector3f::UnitX;
		right = reference.Cross(forward);
	}
	right.Normalize();
	up = forward.Cross(right).GetNormalized();
	CommonUtilities::Matrix4f rigid;
	for (int i = 1; i <= 3; ++i)
	{
		rigid(1, i) = i == 1 ? right.x : i == 2 ? right.y : right.z;
		rigid(2, i) = i == 1 ? up.x : i == 2 ? up.y : up.z;
		rigid(3, i) = i == 1 ? forward.x : i == 2 ? forward.y : forward.z;
		rigid(4, i) = matrix(4, i);
	}
	DirectX::XMFLOAT4X4 matrixData;
	for (int row = 0; row < 4; ++row)
	{
		for (int column = 0; column < 4; ++column)
		{
			matrixData.m[row][column] = rigid(row + 1, column + 1);
		}
	}
	DirectX::XMFLOAT4 rotation;
	DirectX::XMStoreFloat4(&rotation, DirectX::XMQuaternionRotationMatrix(DirectX::XMLoadFloat4x4(&matrixData)));
	myCamera.GetTransform().SetPosition({rigid(4, 1), rigid(4, 2), rigid(4, 3)});
	myCamera.GetTransform().SetRotation({rotation.w, rotation.x, rotation.y, rotation.z});
}
