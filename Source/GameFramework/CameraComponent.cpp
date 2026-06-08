#include "CameraComponent.h"

#include "Actor.h"

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

	const CommonUtilities::Transform& ownerTransform = owner->GetTransform();
	CommonUtilities::Transform& cameraTransform = myCamera.GetTransform();
	cameraTransform.SetPosition(ownerTransform.GetPosition());
	cameraTransform.SetRotation(ownerTransform.GetRotation());
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
