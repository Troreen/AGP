#pragma once

#include "Camera3D.hpp"
#include "InputHandler.h"

class FreeFlyCameraController
{
public:
	FreeFlyCameraController();

	void Init(CommonUtilities::InputHandler& anInputHandler, CommonUtilities::Camera3D& aCamera);
	void Update(float aTimeDelta);
	void ResetMouseLookAnchor();

	void SetMoveSpeed(float aMoveSpeed);
	void SetLookSensitivity(float aLookSensitivity);
	float GetMoveSpeed() const;
	float GetLookSensitivity() const;

private:
	CommonUtilities::InputHandler* myInputHandler;
	CommonUtilities::Camera3D* myCamera;

	float myMoveSpeed;
	float myLookSensitivity;
	float myYawRadians;
	float myPitchRadians;
	float myMaxPitchRadians;
	bool myHasMouseLookAnchor;
};
