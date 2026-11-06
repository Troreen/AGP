#pragma once
#include <GameFramework/Components/CustomComponent.h>
#include <Vector.hpp>

enum class MovementState
{
	Moving,
	Dashing,
	Dodging,
	None

};

class PlayerMovementComponent : public CustomComponent
{
public:

	PlayerMovementComponent();

	void InitializeCustomVariables(const CustomComponentVariables& someVariables) override;
	void BeginPlay() override;

	//Updates
	void Update(float aDeltaTime) override;
	void UpdateNormalMovement(float aDeltaTime);
	void UpdateDash(float aDeltaTime);
	void UpdateDodge(float aDeltaTime);

	//Normal player movement
	void SetDestination(CommonUtilities::Vector3f aDestination);
	void SetWaypoints(std::vector<CommonUtilities::Vector3f>&& someWaypoints);
	void StopMovement();
	void SetAbilityMovementLocked(bool aLocked);
	bool IsAbilityMovementLocked() const;
	void SetScriptedMovement(bool aScripted);
	void RotateTowardsTarget(CommonUtilities::Vector3f aTarget, float aDeltaTime);
	void RotateTowardsDirection(CommonUtilities::Vector3f aDirection, float aDeltaTime);

	bool CanTakeMovementInput() const;
	bool IsMoving() const;

	//Ability Movement
	bool StartDash(const CommonUtilities::Vector3f& aDirection, float aDistance, float aSpeed);
	bool StartDodge(const CommonUtilities::Vector3f& aDirection, float aDistance, float aSpeed);

private:

	std::vector<CommonUtilities::Vector3f> myWaypoints;

	CommonUtilities::Vector3f myDestination;
	CommonUtilities::Vector3f myVelocity;
	float myMaxSpeed;
	bool myHasDestination;
	bool myScriptedMovement = false;
	bool myAbilityMovementLocked = false;
	float myArrivedDistance;
	float myTurnSpeed;

	//Dash
	CommonUtilities::Vector3f myDashDirection;
	float myDashDistance;
	float myDashTimer;
	float myDashSpeed;
	float myDashDuration;
	CommonUtilities::Vector3f myDashStartPosition;

	MovementState myMovementState;
};
