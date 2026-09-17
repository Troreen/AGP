#include "CameraComponent.h"
#include "GameFramework/World/TransformOperations.h"

#include "GameFramework/World/Actor.h"

CameraComponent::CameraComponent(float aHorizontalFieldOfViewDegrees, float aNearPlane, float aFarPlane, const CommonUtilities::Vector2u& aResolution)
	: myCamera(aHorizontalFieldOfViewDegrees, aNearPlane, aFarPlane, aResolution)
{
}

void CameraComponent::Update(float)
{
	SyncCameraToOwner();
}

void CameraComponent::LateUpdate(float)
{
	SyncCameraToOwner();
}

void CameraComponent::SetPerspective(float aHorizontalFieldOfViewDegrees, float aNearPlane, float aFarPlane, const CommonUtilities::Vector2u& aResolution)
{
    EnsureCanMutate();
	myCamera = CommonUtilities::Camera3D(aHorizontalFieldOfViewDegrees, aNearPlane, aFarPlane, aResolution);
	SyncCameraToOwner();
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
    CommonUtilities::Vector3f up{matrix(2,1),matrix(2,2),matrix(2,3)};
    auto right = up.Cross(forward);
    if (right.LengthSqr() < 1e-8f) right = CommonUtilities::Vector3f::UnitX;
    right.Normalize(); up = forward.Cross(right).GetNormalized();
    CommonUtilities::Matrix4f rigid;
    for (int i = 1; i <= 3; ++i)
    {
        rigid(1,i) = i == 1 ? right.x : i == 2 ? right.y : right.z;
        rigid(2,i) = i == 1 ? up.x : i == 2 ? up.y : up.z;
        rigid(3,i) = i == 1 ? forward.x : i == 2 ? forward.y : forward.z;
        rigid(4,i) = matrix(4,i);
    }
    GameFrameworkInternal::SetLocalMatrix(myCamera.GetTransform(), rigid);
}

CommonUtilities::Camera3D& CameraComponent::GetCamera()
{
	SyncCameraToOwner();
	return myCamera;
}

const CommonUtilities::Camera3D& CameraComponent::GetCamera() const
{
	return myCamera;
}
