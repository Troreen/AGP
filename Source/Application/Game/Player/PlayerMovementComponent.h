#pragma once
#include <GameFramework\World\Component.h>
#include <Vector.hpp>

enum class MovementState
{
	Moving,
	Dashing,
	Dodging,
	None

};

class PlayerMovementComponent : public Component
{
public:

	PlayerMovementComponent();

	void BeginPlay() override;

	//Updates
	void Update(float aDeltaTime) override;
	void UpdateNormalMovement(float aDeltaTime);
	void UpdateDash(float aDeltaTime);
	void UpdateDodge(float aDeltaTime);

	//Normal player movement
	void SetDestination(CommonUtilities::Vector3f aDestination);
	void StopMovement();
	void SetScriptedMovement(bool aScripted);
	void RotateTowardsTarget(CommonUtilities::Vector3f aTarget, float aDeltaTime);



	//Ability Movement
	bool CanPerformDash() const;
	void PerformDash();
	void PerformDodgeRoll();

private:

	CommonUtilities::Vector3f myDestination;
	CommonUtilities::Vector3f myVelocity;
	float myMaxSpeed;
	bool myHasDestination;
	bool myScriptedMovement = false;
	float myArrivedDistance;
	float myTurnSpeed;

	MovementState myMovementState;
};
