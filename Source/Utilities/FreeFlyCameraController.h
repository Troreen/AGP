#pragma once

#include "InputHandler.h"
#include "Transform.hpp"

class FreeFlyCameraController
{
public:
	FreeFlyCameraController();

	void Init(CommonUtilities::InputHandler& anInputHandler, CommonUtilities::Transform& aTransform);
	void Update(float aTimeDelta);
	void ResetMouseLookAnchor();

	void SetMoveSpeed(float aMoveSpeed);
	void SetLookSensitivity(float aLookSensitivity);
	float GetMoveSpeed() const;
	float GetLookSensitivity() const;

private:
	CommonUtilities::InputHandler* myInputHandler;
	CommonUtilities::Transform* myTransform;

	float myMoveSpeed;
	float myLookSensitivity;
	float myYawRadians;
	float myPitchRadians;
	float myMaxPitchRadians;
	bool myHasMouseLookAnchor;
};
