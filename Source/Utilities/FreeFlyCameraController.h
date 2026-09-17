#pragma once
#include "Transform.hpp"
#include <vector>

namespace CommonUtilities
{
	struct InputEvent;
}

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
	~FreeFlyCameraController();

	void Init(CommonUtilities::Transform& aTransform);
	void Update(float aTimeDelta);
	void Update(float aTimeDelta, const InputState& anInputState);
	void ResetMouseLookAnchor();

	void SetMoveSpeed(float aMoveSpeed);
	void SetLookSensitivity(float aLookSensitivity);
	float GetMoveSpeed() const;
	float GetLookSensitivity() const;

private:
	CommonUtilities::Transform* myTransform;
	CommonUtilities::Vector3f myMoveBuffer;

	std::vector<unsigned> myInputEventListenerIDs;

	float myMoveSpeed;
	float myLookSensitivity;
	float myYawRadians;
	float myPitchRadians;
	float myMaxPitchRadians;
	bool myHasMouseLookAnchor;

	void HandleLockedMouseMove(const CommonUtilities::InputEvent& anEvent);
};
