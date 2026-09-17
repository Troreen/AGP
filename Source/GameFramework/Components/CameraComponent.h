#pragma once

#include "SceneComponent.h"

#include "Camera3D.hpp"
#include "Vector2.hpp"

// Projection/view data attached to a spatial hierarchy. This component does not
// implement player controls. Select the specific CameraComponent through
// GameContext::SetActiveCamera; the renderer extracts camera data for the snapshot.
class CameraComponent final : public SceneComponent
{
public:
    static constexpr float DefaultFieldOfView = 90.0f;
    static constexpr float DefaultNearPlane = 1.0f;
    static constexpr float DefaultFarPlane = 50000.0f;
    CameraComponent();
	CameraComponent(float aHorizontalFieldOfViewDegrees, float aNearPlane, float aFarPlane, const CommonUtilities::Vector2u& aResolution);

	void Update(float aDeltaTime) override;
	void LateUpdate(float aDeltaTime) override;

    // Invalid projection values are rejected without changing the camera.
	bool SetPerspective(float aHorizontalFieldOfViewDegrees, float aNearPlane, float aFarPlane, const CommonUtilities::Vector2u& aResolution);
	void SyncCameraToOwner();

	CommonUtilities::Camera3D& GetCamera();
	const CommonUtilities::Camera3D& GetCamera() const;

private:
	CommonUtilities::Camera3D myCamera;
};
