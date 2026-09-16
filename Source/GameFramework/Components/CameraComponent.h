#pragma once

#include "Component.h"

#include "Camera3D.hpp"
#include "Vector2.hpp"

// Projection/view data attached to an actor. Moving the owner moves the viewpoint;
// this component does not implement player controls. Select its actor through
// GameContext::SetActiveCamera; the renderer extracts camera data for the snapshot.
class CameraComponent final : public Component
{
public:
	CameraComponent() = default;
	CameraComponent(float aHorizontalFieldOfViewDegrees, float aNearPlane, float aFarPlane, const CommonUtilities::Vector2u& aResolution);

	void Update(float aDeltaTime) override;
	void LateUpdate(float aDeltaTime) override;

	void SetPerspective(float aHorizontalFieldOfViewDegrees, float aNearPlane, float aFarPlane, const CommonUtilities::Vector2u& aResolution);
	void SyncCameraToOwner();

	CommonUtilities::Camera3D& GetCamera();
	const CommonUtilities::Camera3D& GetCamera() const;

private:
	CommonUtilities::Camera3D myCamera;
};
