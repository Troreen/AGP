#pragma once
#include <GameFramework\World\Component.h>
#include <Vector.hpp>

class PlayerMovementComponent : public Component
{
public:

	PlayerMovementComponent();

	void BeginPlay() override;
	void Update(float aDeltaTime) override;

	void SetDestination(CommonUtilities::Vector3f aDestination);
	void RotateTowardsTarget(CommonUtilities::Vector3f aTarget, float aDeltaTime);

private:

	CommonUtilities::Vector3f myDestination;
	CommonUtilities::Vector3f myVelocity;
	float myMaxSpeed;
	bool myHasDestination;
	float myArrivedDistance;
	float myTurnSpeed;
};
