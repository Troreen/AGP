#pragma once
#include "Controller.h"
#include <vector>

class FollowPathController : public Controller
{
public:

	void UpdateActorControllerData(ComponentControllerData& outData) override;

	const CommonUtilities::Vector3f GetDesiredVelocity(ControllerComponent* aComponent) const override;
	const CommonUtilities::Vector3f GetSteeringForce(ControllerComponent* aComponent) const override;

private:
};
