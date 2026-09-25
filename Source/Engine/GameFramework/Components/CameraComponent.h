#pragma once

#include "GameFramework/Components/SceneComponent.h"

#include "Camera3D.hpp"
#include "Vector2.hpp"

class WorldRenderer;

// Projection/view data attached to an Actor. This component does not
// implement player controls. Select the specific CameraComponent through
// World::SetActiveCamera; the engine handles camera extraction.
class CameraComponent final : public SceneComponent
{
public:
	static constexpr float DefaultFieldOfView = 90.0f;
	static constexpr float DefaultNearPlane = 1.0f;
	static constexpr float DefaultFarPlane = 50000.0f;
	CameraComponent();
	CameraComponent(float aHorizontalFieldOfViewDegrees, float aNearPlane, float aFarPlane, const CommonUtilities::Vector2u& aResolution);

	// Invalid projection values are rejected without changing the camera.
	bool SetPerspective(float aHorizontalFieldOfViewDegrees, float aNearPlane, float aFarPlane,
	                    const CommonUtilities::Vector2u& aResolution);
	// Rebuild the projection with the camera's existing field of view and clipping planes.
	bool SetResolution(const CommonUtilities::Vector2u& aResolution);

private:
	void SyncCameraToOwner();
	CommonUtilities::Camera3D myCamera;
	float myHorizontalFieldOfViewDegrees = DefaultFieldOfView;
	float myNearPlane = DefaultNearPlane;
	float myFarPlane = DefaultFarPlane;
	friend class WorldRenderer;
};
