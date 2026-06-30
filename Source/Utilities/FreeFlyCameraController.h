#pragma once

#include "InputHandler.h"
#include "Transform.hpp"

class FreeFlyCameraController
{
public:
	struct InputState
	{
		bool MoveForward = false;
		bool MoveBackward = false;
		bool MoveRight = false;
		bool MoveLeft = false;
		bool MoveUp = false;
		bool MoveDown = false;
		bool MouseLookActive = false;
		float MouseDeltaX = 0.0f;
		float MouseDeltaY = 0.0f;
	};

	FreeFlyCameraController();

	void Init(CommonUtilities::InputHandler& anInputHandler, CommonUtilities::Transform& aTransform);
	void Init(CommonUtilities::Transform& aTransform);
	void Update(float aTimeDelta);
	void Update(float aTimeDelta, const InputState& anInputState);
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
