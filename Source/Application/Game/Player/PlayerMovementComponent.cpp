#include "PlayerMovementComponent.h"
#include <GameFramework/World/Actor.h>
#include <GameFramework/Components/AnimatorComponent.h>
#include <GameFramework/Scenes/CustomComponentVariables.h>

#include <Maths.hpp>
#include <GraphicsEngine/Drawers/LineDrawer.h>
#include <iostream>

PlayerMovementComponent::PlayerMovementComponent()
{
	myMaxSpeed = 0.f;
	myTurnSpeed = 0.0f;
	myHasDestination = false;
	myArrivedDistance = 0.0f;
}

void PlayerMovementComponent::InitializeCustomVariables(const CustomComponentVariables& someVariables)
{
	myMaxSpeed = someVariables.GetOrDefault<float>("Speed", 0.0f);
}

void PlayerMovementComponent::BeginPlay()
{
	myVelocity = { 0.0f, 0.0f, 0.0f };
	myHasDestination = false;
	myArrivedDistance = 10.0f;
	myTurnSpeed = 100.0f;
	myMovementState = MovementState::None;
}

void PlayerMovementComponent::Update(float aDeltaTime)
{
	if (myScriptedMovement || myMovementState == MovementState::None)
	{
		return;
	}


	switch (myMovementState)
	{
		case MovementState::Moving:
			UpdateNormalMovement(aDeltaTime);
			return;
		case MovementState::Dashing:
			UpdateDash(aDeltaTime);
			return;
		case MovementState::Dodging:
			UpdateDodge(aDeltaTime);
			return;
		default:
			std::cout << "You really shouldnt be here... - movement\n";
			return;
	}
}

void PlayerMovementComponent::SetDestination(CommonUtilities::Vector3f aDestination)
{
	if (myScriptedMovement)
	{
		return;
	}
	myDestination = aDestination;
	myHasDestination = true;
	myMovementState = MovementState::Moving;
}

void PlayerMovementComponent::StopMovement()
{
	myMovementState = MovementState::None;
	myDestination = { 0.0f, 0.0f, 0.0f };
	myVelocity = { 0.0f, 0.0f, 0.0f };
	myHasDestination = false;
}

void PlayerMovementComponent::SetScriptedMovement(bool aScripted)
{
	myScriptedMovement = aScripted;
	if (aScripted)
	{
		StopMovement();
	}
}

void PlayerMovementComponent::RotateTowardsTarget(CommonUtilities::Vector3f aTarget, float aDeltaTime)
{
	auto& transform = GetOwner()->GetTransform();

	CommonUtilities::Vector3f direction = (aTarget - transform.GetLocalPosition()).GetNormalized();

	direction.y = 0.0f;

	float yawAngleRadians = std::atan2(direction.x, direction.z);

	float yawAngleDegrees = CommonUtilities::RadiansToDegrees(yawAngleRadians);

	transform.SetLocalRotationDegrees({ yawAngleDegrees, 0.0f, 0.0f });
}

void PlayerMovementComponent::UpdateNormalMovement(float aDeltaTime)
{
	if (AnimatorComponent* animation = GetOwner()->GetComponent<AnimatorComponent>())
	{
		animation->GetTree().SetBool("IsRunning", true);
	}

	CommonUtilities::Vector3f position = GetOwner()->GetTransform().GetLocalPosition();

	CommonUtilities::Vector3f direction = myDestination - position;

	float distance = (myDestination - position).Length();

	if (distance <= myArrivedDistance)
	{
		if (AnimatorComponent* animation = GetOwner()->GetComponent<AnimatorComponent>())
		{
			animation->GetTree().SetBool("IsRunning", false);
		}
		myDestination = { 0.0f, 0.0f, 0.0f };
		myHasDestination = false;
		myMovementState = MovementState::None;
		return;
	}

	LineDrawer& lineDrawer = LineDrawer::Get();
	lineDrawer.DrawLine(position, myDestination, { 0.0f, 1.0f, 0.0f, 1.0f });

	direction = direction.GetNormalized();

	myVelocity = direction * myMaxSpeed;

	position += myVelocity * aDeltaTime;

	GetOwner()->GetTransform().SetLocalPosition(position);

	RotateTowardsTarget(myDestination, aDeltaTime);
}

void PlayerMovementComponent::UpdateDash(float aDeltaTime)
{
	aDeltaTime;
}

void PlayerMovementComponent::UpdateDodge(float aDeltaTime)
{
	aDeltaTime;
}

bool PlayerMovementComponent::CanPerformDash() const
{
	return myMovementState != MovementState::Dodging;
}

void PlayerMovementComponent::PerformDash()
{
	myMovementState = MovementState::Dashing;
}

void PlayerMovementComponent::PerformDodgeRoll()
{
	myMovementState = MovementState::Dodging;
}
