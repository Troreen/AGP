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
	FreeFlyCameraController();
	~FreeFlyCameraController();

	void Init(CommonUtilities::Transform& aTransform);
	void Update(float aTimeDelta);
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
